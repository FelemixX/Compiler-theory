/**
 * Анализатор рекурсивного спуска для целочисленных выражений, который может включать переменные и вызовы функций.
 */
#include <setjmp.h> //либа для создания переходов а-ля ассемблер
#include <ctype.h>
#include <stdlib.h>
#include <cstring>
#include <stdio.h>
#include <iostream>

#include "enum.h"
#include "const.h"
using namespace std;
/// Совместимость с защищенными функциями
#if !defined(_MSC_VER) || _MSC_VER < 1400
#define strcpy_s(dest, count, source) strncpy((dest), (source), (count))
#endif
/**
 * @brief Структура переменных
 */
struct variable_type
{
	char variable_name[ID_LEN];
	int variable_type;
	int variable_value;
};
/**
 * @brief Структура функции
 */
struct function_type
{
	char func_name[ID_LEN];
	int ret_type;
	char *loc;
};
/**
 * @brief Структура списка зарезервированных слов
 */
struct commands
{
	char command[20];
	char tok;
};
/// Структура списка функций стандартной библиотеки
struct intern_func_type
{
	char *f_name;	/* имя функции */
	int (*p)(void); /* указатель на функцию */
};

extern variable_type global_vars[NUM_GLOBAL_VARS];
extern function_type func_stack[NUMBER_FUNCTIONS];
extern commands table_with_statements[];
/// Здесь функции "стандартной библиотеки" объявлены таким образом, что их можно поместить во внутренюю таблицу функции
int call_getche(void);
int call_putch(void);
int call_puts(void);
int print(void);
int getnum(void);

/// Массив стандартных функций
intern_func_type intern_func[] = {
	{"getche", call_getche},
	{"putch", call_putch},
	{"puts", call_puts},
	{"print", print},
	{"getnum", getnum},
	{"", 0} /* этот список заканчивается нулем */
};

extern char *source_code_location; /* текущее положение в исходном тексте программы */
extern char *program_start_buffer; /* указатель на начало буфера программы */
extern jmp_buf execution_buffer;   /* содержит данные для longjmp() */
extern char current_token[80];	   /* строковое представление current_token */
extern char token_type;			   /* содержит тип current_token */
extern char current_tok_datatype;  /* внутреннее представление current_token */
extern int ret_value;			   /* возвращаемое значение функции */

/// Функции для анализатора
void eval_expression(int *value);
void eval_assignment_expression(int *value);
void eval_comparison_expression(int *value);		 /* обработка операторов отношений */
void eval_sum_minus_expression(int *value);			 /* обработка сложения или вычитания */
void eval_multiply_division_expression(int *value);	 /* обработка умножения, деления, целочисленного деления */
void eval_unar_minus_expression(int *value);		 /* унарные плюс и минус */
void eval_exp_in_parenthesis_expression(int *value); /* обработка выражений в скобках */
void atom(int *value);								 /* найти значение числа, переменной или функции */
char get_next_token(void);
void shift_source_code_location_back(void);
char look_up_token_in_table(char *s);
int internal_func(char *s);
int is_delimiter(char c);
int is_whitespace(char c);
static void str_replace(char *line, const char *search, const char *replace);

#if defined(_MSC_VER) && _MSC_VER >= 1200
__declspec(noreturn) void syntax_error(int error);
#elif __GNUC__
void syntax_error(int error) __attribute((noreturn));
#else
void syntax_error(int error);
#endif

/// Функции из интерпретатора
void assign_var(char *var_name, int value);
int find_var(char *s);
int is_variable(char *s);
char *find_function_in_function_table(char *name);
void call_function(void);

/**
 * @brief Точка входа в синтаксический анализатор выражений
 */
void eval_expression(int *value)
{
	get_next_token();
	if (!*current_token)
	{
		syntax_error(NO_EXP);
		return;
	}
	if (*current_token == ';')
	{
		*value = 0; /* пустое выражение */
		return;
	}
	eval_assignment_expression(value);
	shift_source_code_location_back(); /* возврат последней лексемы во входной поток */
}
/**
 * @brief Обработка выражения в присваивании
 * @param value
 */
void eval_assignment_expression(int *value)
{
	char temp[ID_LEN]; /* Содержит имя переменной, которой присваивается значение */
	char temp_tok;

	if (token_type == VARIABLE)
	{
		/// Если встретили переменную, то проверяем, присваивается ли ей какое-либо значение */
		if (is_variable(current_token))
		{
			strcpy_s(temp, ID_LEN, current_token);
			temp_tok = token_type;
			get_next_token();
			if (*current_token == '=') /* Если присваивается */
			{
				get_next_token();
				eval_assignment_expression(value); /* то смотрим, что надо присвоить */
				assign_var(temp, *value);		   /* присваиваем */
				return;
			}
			else
			{									   /* Если не присваивается */
				shift_source_code_location_back(); /* То забываем про temp и копируем изначальное значение токена из temp_tok */
				strcpy_s(current_token, 80, temp);
				token_type = temp_tok;
			}
		}
	}
	eval_comparison_expression(value);
}
/**
 * @brief Обработка операций сравнения.
 * @param value
 */
void eval_comparison_expression(int *value)
{
	int partial_value;
	char op;
	char relational_operators[7] = {
		LOWER,
		LOWER_OR_EQUAL,
		GREATER,
		GREATER_OR_EQUAL,
		EQUAL,
		NOT_EQUAL,
		0};

	eval_sum_minus_expression(value);
	op = *current_token;
	if (strchr(relational_operators, op))
	{
		get_next_token();
		eval_sum_minus_expression(&partial_value);
		switch (op)
		{ /* выполнения операторов отношений */
		case LOWER:
			*value = *value < partial_value;
			break;
		case LOWER_OR_EQUAL:
			*value = *value <= partial_value;
			break;
		case GREATER:
			*value = *value > partial_value;
			break;
		case GREATER_OR_EQUAL:
			*value = *value >= partial_value;
			break;
		case EQUAL:
			*value = *value == partial_value;
			break;
		case NOT_EQUAL:
			*value = *value != partial_value;
			break;
		}
	}
}

/**
 * @brief Суммирование или вычисление двух термов
 * @param value
 */
void eval_sum_minus_expression(int *value)
{
	char op;
	int partial_value;

	eval_multiply_division_expression(value);
	while ((op = *current_token) == '+' || op == '-')
	{
		get_next_token();
		eval_multiply_division_expression(&partial_value);
		switch (op)
		{ /* add or subtract */
		case '-':
			*value = *value - partial_value;
			break;
		case '+':
			*value = *value + partial_value;
			break;
		}
	}
}
/**
 * @brief Умножение или деление двух множителей
 * @param value
 */
void eval_multiply_division_expression(int *value)
{
	char op;
	int partial_value, t;

	eval_unar_minus_expression(value);
	while ((op = *current_token) == '*' || op == '/' || op == '%')
	{
		get_next_token();
		eval_unar_minus_expression(&partial_value);
		switch (op)
		{ /* умножение, деление, или mod */
		case '*':
			*value = *value * partial_value;
			break;
		case '/':
			if (partial_value == 0)
				syntax_error(DIV_BY_ZERO);
			*value = (*value) / partial_value;
			break;
		case '%':
			t = (*value) / partial_value;
			*value = *value - (t * partial_value);
			break;
		}
	}
}
/**
 * @brief Унарный + или -
 * @param value
 */
void eval_unar_minus_expression(int *value)
{
	char op;

	op = '\0';
	if (*current_token == '+' || *current_token == '-')
	{
		op = *current_token;
		get_next_token();
	}
	eval_exp_in_parenthesis_expression(value);
	if (op)
		if (op == '-')
			*value = -(*value);
}

/**
 * @brief Обработка выражения в скобках
 * @param value
 */
void eval_exp_in_parenthesis_expression(int *value)
{
	if (*current_token == '(')
	{
		get_next_token();
		eval_assignment_expression(value); /* получить подстроку */
		if (*current_token != ')')
			syntax_error(PAREN_EXPECTED);
		get_next_token();
	}
	else
		atom(value);
}
/**
 * @brief Получение значения числа, переменной или функции
 * @param value
 */
void atom(int *value)
{
	int i;

	switch (token_type)
	{
	case VARIABLE:
		i = internal_func(current_token);
		if (i != -1)
		{ /* вызов стандартной функции */
			*value = (*intern_func[i].p)();
		}
		else if (find_function_in_function_table(current_token))
		{ /* вызов пользовательской функции */
			call_function();
			*value = ret_value;
		}
		else
			*value = find_var(current_token); /* получаем значение переменной */
		get_next_token();
		return;
	case NUMBER: /* числовая константа */
		*value = atoi(current_token);
		get_next_token();
		return;
	case DELIMITER: /* символьная константа */
		if (*current_token == '\'')
		{
			*value = *source_code_location;
			source_code_location++;
			if (*source_code_location != '\'')
				syntax_error(QUOTE_EXPECTED);
			source_code_location++;
			get_next_token();
			return;
		}
		if (*current_token == ')')
			return; /* пустое выражение в скобках */
		else
			syntax_error(SYNTAX); /* синтаксическая ошибка */
	default:
		syntax_error(SYNTAX); /* синтаксическая ошибка */
	}
}
/**
 * @brief Печать ошибки
 *
 * Выводит сообщение об ошибке, основываясь на полученном типе ошибки
 * @param error_type
 */
void syntax_error(int error_type)
{
	char *program_pointer_location, *temp;
	int line_count = 0;
	int i;

	string errors_human_readable[]{
		"Синтаксическая ошибка",
		"Слишком много или мало скобок",
		"Нет выражения",
		"Не хватает знаков равно",
		"Не является переменной",
		"Неправильно задан параметр функции",
		"Не хватает точки с запятой",
		"Слишком много или мало операторных скобок",
		"Функция не определена",
		"Тип данных не указан",
		"Слишком много обращений к вложенным функциям",
		"Функция возвращает значения без обращения к ней",
		"Не хватает скобки",
		"Нет оператора для цикла while",
		"Не хватает закрывающих кавычек",
		"Не является строкой",
		"Слишком много локальных переменных",
		"На ноль делить НЕЛЬЗЯ"};

	cout << "\n"
		 << errors_human_readable[error_type];

	program_pointer_location = program_start_buffer;

	/// Поиск позиции ошибки
	while (program_pointer_location != source_code_location && *program_pointer_location != '\0')
	{
		program_pointer_location++;
		if (*program_pointer_location == '\r')
		{
			line_count++;
			if (program_pointer_location == source_code_location)
			{
				break;
			}

			program_pointer_location++;

			if (*program_pointer_location != '\n')
			{
				program_pointer_location--;
			}
		}
		else if (*program_pointer_location == '\n')
		{
			line_count++;
		}
		else if (*program_pointer_location == '\0')
		{
			line_count++;
		}
	}
	cout << " на строке " << line_count << endl;

	/// Поиск и указание последнего рабочего кусочка кода
	temp = program_pointer_location--;
	for (i = 0; i < 20 && program_pointer_location > program_start_buffer && *program_pointer_location != '\n' && *program_pointer_location != '\r'; i++, program_pointer_location--)
		;
	for (i = 0; i < 30 && program_pointer_location <= temp; i++, program_pointer_location++)
		printf("%c", *program_pointer_location);

	longjmp(execution_buffer, 1);
}
/**
 * @brief Считывание лексемы из входного потока
 * @return
 */
char get_next_token(void)
{
	char *temp_token;

	token_type = 0;
	current_tok_datatype = 0;

	temp_token = current_token;
	*temp_token = '\0';

	/// Пропуск пробелов, символов табуляции и пустой строки
	while (is_whitespace(*source_code_location) && *source_code_location)
		++source_code_location;

	/* Обработка символа переноса на новую строку для Mac и Windows */
	if (*source_code_location == '\r')
	{
		++source_code_location;
		/* Пропускает только \n если мы на Windows, а не на Mac */
		if (*source_code_location == '\n')
		{
			++source_code_location;
		}
		/* Пропуск пробела в коде */
		while (is_whitespace(*source_code_location) && *source_code_location)
			++source_code_location;
	}

	/* Обработка новых строк для Unix подобных систем */
	if (*source_code_location == '\n')
	{
		++source_code_location;
		/* Пропустить пробел */
		while (is_whitespace(*source_code_location) && *source_code_location)
			++source_code_location;
	}

	/// Конец файла
	if (*source_code_location == '\0')
	{
		*current_token = '\0';
		current_tok_datatype = FINISHED;
		return (token_type = DELIMITER);
	}
	/// Ограничение блока
	if (strchr("{}", *source_code_location))
	{ /* block delimiters */
		*temp_token = *source_code_location;
		temp_token++;
		*temp_token = '\0';
		source_code_location++;
		return (token_type = BLOCK);
	}
	/// Поиск комментариев
	if (*source_code_location == '/')
		if (*(source_code_location + 1) == '*') /* найти конец комментария  */
		{
			source_code_location += 2;
			do
			{
				while (*source_code_location != '*' && *source_code_location != '\0')
					source_code_location++;
				if (*source_code_location == '\0')
				{
					source_code_location--;
					break;
				}
				source_code_location++;
			} while (*source_code_location != '/');
			source_code_location++;
		}
	/// Поиск комментариев C++ стиля
	if (*source_code_location == '/')
		if (*(source_code_location + 1) == '/')
		{ /* Это комментарий */
			source_code_location += 2;
			/* Поиск конца файла */
			while (*source_code_location != '\r' && *source_code_location != '\n' && *source_code_location != '\0')
				source_code_location++;
			if (*source_code_location == '\r' && *(source_code_location + 1) == '\n')
			{
				source_code_location++;
			}
		}
	/// Поиск конец файла после комментария
	if (*source_code_location == '\0')
	{ /* Поиск конца файла */
		*current_token = '\0';
		current_tok_datatype = FINISHED;
		return (token_type = DELIMITER);
	}
	/// Операции отношений
	if (strchr("!<>=", *source_code_location))
	{
		switch (*source_code_location)
		{
		case '=':
			if (*(source_code_location + 1) == '=')
			{
				source_code_location++;
				source_code_location++;
				*temp_token = EQUAL;
				temp_token++;
				*temp_token = EQUAL;
				temp_token++;
				*temp_token = '\0';
			}
			break;
		case '!':
			if (*(source_code_location + 1) == '=')
			{
				source_code_location++;
				source_code_location++;
				*temp_token = NOT_EQUAL;
				temp_token++;
				*temp_token = NOT_EQUAL;
				temp_token++;
				*temp_token = '\0';
			}
			break;
		case '<':
			if (*(source_code_location + 1) == '=')
			{
				source_code_location++;
				source_code_location++;
				*temp_token = LOWER_OR_EQUAL;
				temp_token++;
				*temp_token = LOWER_OR_EQUAL;
			}
			else
			{
				source_code_location++;
				*temp_token = LOWER;
			}
			temp_token++;
			*temp_token = '\0';
			break;
		case '>':
			if (*(source_code_location + 1) == '=')
			{
				source_code_location++;
				source_code_location++;
				*temp_token = GREATER_OR_EQUAL;
				temp_token++;
				*temp_token = GREATER_OR_EQUAL;
			}
			else
			{
				source_code_location++;
				*temp_token = GREATER;
			}
			temp_token++;
			*temp_token = '\0';
			break;
		}
		if (*current_token)
			return (token_type = DELIMITER);
	}
	/// Разделитель
	if (strchr("+-*^/%=;(),'", *source_code_location))
	{
		*temp_token = *source_code_location;
		source_code_location++; /* продвижение на следующую позицию */
		temp_token++;
		*temp_token = '\0';
		return (token_type = DELIMITER);
	}
	/// Строка в кавычках
	if (*source_code_location == '"')
	{
		source_code_location++;
		while ((*source_code_location != '"' &&
				*source_code_location != '\r' &&
				*source_code_location != '\n' &&
				*source_code_location != '\0') ||
			   (*source_code_location == '"' &&
				*(source_code_location - 1) == '\\'))
			*temp_token++ = *source_code_location++;

		if (*source_code_location == '\r' || *source_code_location == '\n' || *source_code_location == '\0')
			syntax_error(SYNTAX);
		source_code_location++;
		*temp_token = '\0';
		str_replace(current_token, "\\a", "\a");
		str_replace(current_token, "\\b", "\b");
		str_replace(current_token, "\\f", "\f");
		str_replace(current_token, "\\n", "\n");
		str_replace(current_token, "\\r", "\r");
		str_replace(current_token, "\\t", "\t");
		str_replace(current_token, "\\v", "\v");
		str_replace(current_token, "\\\\", "\\");
		str_replace(current_token, "\\\'", "\'");
		str_replace(current_token, "\\\"", "\"");
		return (token_type = STRING);
	}
	/// Число
	if (isdigit((int)*source_code_location))
	{
		while (!is_delimiter(*source_code_location))
			*temp_token++ = *source_code_location++;
		*temp_token = '\0';
		return (token_type = NUMBER);
	}
	/// Переменная или оператор
	if (isalpha((int)*source_code_location))
	{
		while (!is_delimiter(*source_code_location))
			*temp_token++ = *source_code_location++;
		token_type = TEMP;
	}

	*temp_token = '\0';
	/// Эта строка является оператором или переменной?
	if (token_type == TEMP)
	{
		current_tok_datatype = look_up_token_in_table(current_token); /* преобразовать во внутренее представление */
		if (current_tok_datatype)									  /* это зарезервированное слово */
			token_type = KEYWORD;
		else
			token_type = VARIABLE;
	}
	return token_type;
}
/**
 * @brief Возврат лексемы во входной поток.
 *
 * Передвигаем указатель на текущую программу на *_токен_* обратно
 */
void shift_source_code_location_back(void)
{
	char *t;

	t = current_token;
	for (; *t; t++)
		source_code_location--;
}
/**
 * @brief Поиск внутреннего представления лексемы в таблице лексем
 * @param token_string
 * @return
 */
char look_up_token_in_table(char *token_string)
{
	int i;
	char *pointer_to_token_string;

	/* переводим токен в нижний регистр */
	pointer_to_token_string = token_string;
	while (*pointer_to_token_string)
	{
		*pointer_to_token_string = (char)tolower(*pointer_to_token_string);
		pointer_to_token_string++;
	}

	/* проверяем есть ли данный токен в таблице специальных зарезервированных слов. */
	for (i = 0; *table_with_statements[i].command; i++)
	{
		if (!strcmp(table_with_statements[i].command, token_string))
			return table_with_statements[i].tok;
	}
	return 0; /* unknown command */
}
/**
 * @brief Возвращает индекс функции во внутренней библиотеке, или -1, если не найдена.
 * @param s
 * @return
 */
int internal_func(char *s)
{
	int i;

	for (i = 0; intern_func[i].f_name[0]; i++)
	{
		if (!strcmp(intern_func[i].f_name, s))
			return i;
	}
	return -1;
}
/**
 * @brief Разделитель
 *
 * Проверяет является ли символ разделителем
 *
 * @param Символ
 *
 * @return 1 - разделитель
 * 0 - не разделитель
 */
int is_delimiter(char c)
{
	if (strchr(" !;,+-<>'/*%^=()", c) || c == 9 ||
		c == '\r' || c == '\n' || c == 0)
		return 1;
	return 0;
}
/**
 * @brief Пробел/таб
 * @param c
 * @return Возвращает 1, если с - пробел или табуляция
 */
int is_whitespace(char c)
{
	if (c == ' ' || c == '\t')
		return 1;
	else
		return 0;
}
/**
 * Модификация на месте находит и заменяет строку.
 * Предполагается, что буфер, на который указывает строка, достаточно велик для хранения результирующей строки
 * @param line
 * @param search
 * @param replace
 */
static void str_replace(char *line, const char *search, const char *replace)
{
	char *sp;
	while ((sp = strstr(line, search)) != NULL)
	{
		int search_len = (int)strlen(search);
		int replace_len = (int)strlen(replace);
		int tail_len = (int)strlen(sp + search_len);

		memmove(sp + replace_len, sp + search_len, tail_len + 1);
		memcpy(sp, replace, replace_len);
	}
}

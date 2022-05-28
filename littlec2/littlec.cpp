/**
 * Интерпретатор языка Little C
 */
#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>

#include "const.h"
#include "enum.h"
using namespace std;

/// Совместимость с защищенными функциями
#if !defined(_MSC_VER) || _MSC_VER < 1400
#define strcpy_s(dest, count, source) strncpy((dest), (source), (count))
#define fopen_s(pFile, filename, mode) (((*(pFile)) = fopen((filename), (mode))) == NULL)
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
/// Массив локальных переменных
variable_type local_var_stack[NUM_LOCAL_VARS];
/// Массив глобальных переменных
variable_type global_vars[NUM_GLOBAL_VARS];
/// Массив функций
function_type function_table[NUMBER_FUNCTIONS];
/// Список зарезервированных слов
commands table_with_statements[] = {
	{"if", IF},
	{"else", ELSE},
	{"for", FOR},
	{"do", DO},
	{"while", WHILE},
	{"char", CHAR},
	{"int", INT},
	{"return", RETURN},
	{"continue", CONTINUE},
	{"break", BREAK},
	{"end", END},
	{"", END},
};

char *source_code_location;			   /* текущее положение в исходном тексте программы */
char *program_start_buffer;			   /* указатель на начало буфера программы */
jmp_buf execution_buffer;			   /* содержит данные для longjmp() */
int call_stack[NUMBER_FUNCTIONS];	   /* массив вызванных функций  */
char current_token[80];				   /* строковое представление current_token */
char token_type;					   /* содержит тип current_token */
char current_tok_datatype;			   /* внутреннее представление current_token */
int function_last_index_on_call_stack; /* индекс вершины стека вызова функции */
int pos_in_funcition;				   /* индекс в таблице функций */
int global_variable_position;		   /* индекс глобальной переменной в таблице global_vars */
int local_var_to_stack;				   /* индекс в стеке локальных переменных */
int ret_value;						   /* возвращаемое значение функции */
int ret_occurring;					   /* возврат функции */
int break_occurring;				   /* разрыв цикла */
/// Функции из интерпретатора
void execute(char *fileName);
void interpret_block();
int load_program(char *p, char *fname);
void prescan_source_code();
void declare_global_variables();
void declare_local_variables();
void call_function();
char *find_function_in_function_table(char *name);
void get_function_parameters();
void get_function_arguments();
void function_return();
void local_push(struct variable_type local_variable);
int func_pop();
void function_push_variables_on_call_stack(int i);
void assign_var(char *var_name, int value);
int find_var(char *s);
int is_variable(char *s);
void execute_if_statement();
void exec_while();
void exec_do();
void find_eob();
void exec_for();

/// Функции из парсера
void print();
void shift_source_code_location_back();
void eval_expression(int *value);
void syntax_error(int error);
char get_next_token();

int main(int argc, char *argv[])
{
	string file;
	char *file_name;
	// если мы запустили данную программу через консоль
	// и передали в качестве параметра имя файла с программой
	if (argc == 2)
	{
		strcpy(file_name, argv[1]);
	}
	// Также мы можем ввести название программы через консоль вручную
	else
	{
		cout << "Файл: ";
		cin >> file;
		file_name = new char[file.length() + 1];
		strcpy(file_name, file.c_str());
	}

	execute(file_name);

	return 0;
}
/**
 * @brief Выполнить интерпретация и исполнить код
 * @param fileName
 */
void execute(char *fileName)
{
	/// Если названия файла нет - выход
	if (fileName[0] == ' ')
	{
		cout << "Пустое имя файла" << endl;
		exit(1);
	}
	/// Выделить память под программу PROG_SIZE - размер программы, не получилось - выход
	if ((program_start_buffer = (char *)malloc(PROG_SIZE)) == nullptr)
	{
		cout << "Сбой распределения памяти" << endl;
		exit(1);
	}
	/// Загрузить программу для выполнения
	if (!load_program(program_start_buffer, fileName))
	{
		cout << "Не удалось считать код" << endl;
		exit(1);
	}
	/// Инициализация буфера longjump
	if (setjmp(execution_buffer))
	{
		exit(1);
	}
	/// Инициализация индекса глобальных переменных
	global_variable_position = 0;
	/// Установка указателя на начало буфера программы
	source_code_location = program_start_buffer;
	/// Определение адресов всех функций и глобальных переменных
	prescan_source_code();
	/// Инициализация индекса стека локальных переменных
	local_var_to_stack = 0;
	/// Инициализация индекса стека вызова CALL
	function_last_index_on_call_stack = 0;
	/// initialize the break occurring flag
	break_occurring = 0;
	/// Вызываем функцию main она всегда вызывается первой
	source_code_location = find_function_in_function_table("main");
	/// main написан с ошибкой или отсутствует
	if (!source_code_location)
	{
		cout << "\"main\" не найдено или написано с ошибкой" << endl;
		exit(1);
	}
	/// Возвращаемся к открывающей (
	source_code_location--;
	strcpy_s(current_token, 80, "main");
	/// Вызываем main и интерпретируем
	call_function();
}
/**
 * @brief Интерпретация одного оператора или блока
 *
 * Когда interp_block() возвращает управление после первого вызова, в main() встретилась последняя закрывающаяся фигурная скобка или оператор return.
 */
void interpret_block()
{
	int value;
	char is_block_open = 0;

	do
	{
		token_type = get_next_token();
		/// При интерпретации одного оператора возврат после первой точки с запятой.

		/// Определение типа лексемы
		if (token_type == VARIABLE) /* Это не зарегистрированное слово, обрабатывается выражение. */
		{
			shift_source_code_location_back(); /* возврат лексемы во входной поток для дальнейшей обработки функцией eval_exp() */
			eval_expression(&value);		   /* обработка выражения */
			if (*current_token != ';')
				syntax_error(SEMICOLON_EXPECTED);
		}
		else if (token_type == BLOCK) /* Это ограничитель блока */
		{
			if (*current_token == '{') /* Блок */
				is_block_open = 1;	   /* Интерпретация блока, а не оператора */
			else
				return; /* Это }, выход */
		}
		else /* Зарезервированное слово */
			switch (current_tok_datatype)
			{
			case CHAR:
			case INT: /* объявление локальной переменной */
				shift_source_code_location_back();
				declare_local_variables();
				break;
			case RETURN: /* возврат из вызова функции */
				function_return();
				ret_occurring = 1;
				return;
			case CONTINUE: /* обработка оператора continue */
				return;
			case BREAK: /* выход из цикла */
				break_occurring = 1;
				return;
			case IF: /* обработка оператора if */
				execute_if_statement();
				if (ret_occurring > 0 || break_occurring > 0)
				{
					return;
				}
				break;
			case ELSE:		/* обработка оператора else */
				find_eob(); /* поиск конца блока else и продолжение выполнения */
				break;
			case WHILE: /* обработка цикла while */
				exec_while();
				if (ret_occurring > 0)
				{
					return;
				}
				break;
			case DO: /* обработка цикла do-while */
				exec_do();
				if (ret_occurring > 0)
				{
					return;
				}
				break;
			case FOR: /* обработка цикла for */
				exec_for();
				if (ret_occurring > 0)
				{
					return;
				}
				break;
			case END: /* Конец uwu */
				exit(0);
			}
	} while (current_tok_datatype != FINISHED && is_block_open);
}
/**
 * Считать файл с кодом и внести в память
 * @param p
 * @param fname
 */
int load_program(char *p, char *fname)
{
	FILE *fp;
	int i;

	if (fopen_s(&fp, fname, "rb") != 0 || fp == NULL)
		return 0;

	i = 0;
	do
	{
		*p = (char)getc(fp);
		p++;
		i++;
	} while (!feof(fp) && i < PROG_SIZE);

	if (*(p - 2) == 0x1a) // рудимент из бейсика. Ставится в конце исполняемого файла
		*(p - 2) = '\0';  /* конец строки завершает программу */
	else
		*(p - 1) = '\0';
	fclose(fp);
	return 1;
}
/**
 * @brief Предварительный проход по коду
 *
 * Найти адреса всех функций и запомнить глобальные переменные
 */
void prescan_source_code()
{
	char *initial_source_code_location, *temp_source_code_location;
	char temp_token[ID_LEN + 1];
	int datatype;
	/// Если is_brace_open = 0, о текущая позиция указателя программы находится в не какой-либо функции
	int is_brace_open = 0;

	initial_source_code_location = source_code_location;
	pos_in_funcition = 0;
	do
	{
		while (is_brace_open) /* обхода кода функции внутри фигурных скобок */
		{
			get_next_token();
			if (*current_token == '{') /* когда встречаем открывающую скобку, увеличиваем is_brace_open на один */
				is_brace_open++;
			if (*current_token == '}')
				is_brace_open--; /* когда встречаем закрывающую уменьшаем на один */
		}

		temp_source_code_location = source_code_location; /* запоминаем текущую позицию */
		get_next_token();

		/// тип глобальной переменной или возвращаемого значения функции
		if (current_tok_datatype == CHAR || current_tok_datatype == INT)
		{
			datatype = current_tok_datatype; /* сохраняем тип данных */
			get_next_token();
			if (token_type == VARIABLE)
			{
				//
				strcpy_s(temp_token, ID_LEN + 1, current_token);
				get_next_token();
				if (*current_token != '(') /* должно быть глобальной переменной */
				{
					source_code_location = temp_source_code_location; /* вернуться в начало объявления */
					declare_global_variables();
				}
				else if (*current_token == '(') /* должно быть функцией */
				{
					function_table[pos_in_funcition].loc = source_code_location;
					function_table[pos_in_funcition].ret_type = datatype;
					strcpy_s(function_table[pos_in_funcition].func_name, ID_LEN, temp_token);
					pos_in_funcition++;
					while (*source_code_location != ')')
						source_code_location++;
					source_code_location++;
					/* сейчас source_code_location указывает на открывающуюся фигурную скобку функции */
				}
				else
					shift_source_code_location_back();
			}
		}
		else if (*current_token == '{')
			is_brace_open++;
	} while (current_tok_datatype != FINISHED);
	source_code_location = initial_source_code_location;
}
/**
 * @brief Объявление глобальной переменной
 *
 * Данные хранятся в списке global vars
 */
void declare_global_variables()
{
	int variable_type;
	get_next_token();					  /* получаем тип данных */
	variable_type = current_tok_datatype; /* запоминаем тип данных */

	/// Обработка списка переменных
	do
	{
		global_vars[global_variable_position].variable_type = variable_type;
		global_vars[global_variable_position].variable_value = 0; /* инициализируем нулем */
		get_next_token();										  /* определяем имя */
		strcpy_s(global_vars[global_variable_position].variable_name, ID_LEN, current_token);
		get_next_token();
		global_variable_position++;
	} while (*current_token == ',');

	if (*current_token != ';')
		syntax_error(SEMICOLON_EXPECTED);
}
/**
 * @brief Объявление локальной переменной
 *
 * Добавляем local_var_stack через local_push
 */
void declare_local_variables()
{
	struct variable_type local_variable;

	/// Получить типа
	get_next_token();

	local_variable.variable_type = current_tok_datatype;
	local_variable.variable_value = 0; /* Инициализация нулем 0 */

	/// Обработка списка переменных
	do
	{
		get_next_token(); /* определение имени */
		strcpy_s(local_variable.variable_name, ID_LEN, current_token);
		local_push(local_variable);
		get_next_token();
	} while (*current_token == ',');

	if (*current_token != ';')
		syntax_error(SEMICOLON_EXPECTED);
}
/**
 * @brief Вызов и выполнение функции
 */
void call_function()
{
	char *function_location, *temp_source_code_location;
	int local_var_temp;

	function_location = find_function_in_function_table(current_token); /* найти точку входа функции */
	if (function_location == NULL)
		syntax_error(FUNC_UNDEFINED); /* функция не определена */
	else
	{
		local_var_temp = local_var_to_stack;				   /* запоминание индекса стека локальных переменных */
		get_function_arguments();							   /* получение аргумента функции */
		temp_source_code_location = source_code_location;	   /* запоминание адреса возврата */
		function_push_variables_on_call_stack(local_var_temp); /* запоминание индекса стека локальных переменных */
		source_code_location = function_location;			   /* переустановка source_code_location в начало функции */
		ret_occurring = 0;									   /* P возвращаемая возникающая переменная */
		get_function_parameters();							   /* загрузка параметров функции значениями аргументов */
		interpret_block();									   /* интерпретация функции */
		ret_occurring = 0;									   /* обнуление возвращаемой переменной */
		source_code_location = temp_source_code_location;	   /* восстановление initial_source_code_location */
		local_var_to_stack = func_pop();					   /* сброс стека локальных переменных */
	}
}
/**
 * @brief Возвращает точку входа указанной функции
 * @param name
 * @return NULL if not found
 */
char *find_function_in_function_table(char *name)
{
	int pos_in_func;

	for (pos_in_func = 0; pos_in_func < pos_in_funcition; pos_in_func++)
		if (!strcmp(name, function_table[pos_in_func].func_name))
			return function_table[pos_in_func].loc;

	return nullptr;
}
/**
 * @brief Заталкивание аргументов функций в стек локальных переменных
 */
void get_function_arguments()
{
	int value, count, temp[NUM_PARAMS];
	struct variable_type i;

	count = 0;
	get_next_token();
	if (*current_token != '(')
		syntax_error(PAREN_EXPECTED);

	/// Обработка списка значений
	do
	{
		eval_expression(&value);
		temp[count] = value; /* временное запоминание */
		get_next_token();
		count++;
	} while (*current_token == ',');
	count--;

	/// Затолкнуть в local_var_stack в обратном порядке
	for (; count >= 0; count--)
	{
		i.variable_value = temp[count];
		i.variable_type = ARG;
		local_push(i);
	}
}
/**
 * @brief Получение параметров функции
 */
void get_function_parameters()
{
	struct variable_type *variable_type_pointer;
	int position;

	position = local_var_to_stack - 1;

	/// Обработка списка параметров
	do
	{
		get_next_token();
		variable_type_pointer = &local_var_stack[position];
		if (*current_token != ')')
		{
			if (current_tok_datatype != INT && current_tok_datatype != CHAR)
				syntax_error(TYPE_EXPECTED);

			variable_type_pointer->variable_type = token_type;
			get_next_token();

			/// Связывание имени параметров с аргументом, уже находящимся в стеке локальных переменных
			strcpy_s(variable_type_pointer->variable_name, ID_LEN, current_token);
			get_next_token();
			position--;
		}
		else
			break;
	} while (*current_token == ',');
	if (*current_token != ')')
		syntax_error(PAREN_EXPECTED);
}
/**
 * @brief Возврат из функции
 */
void function_return()
{
	int value = 0;
	/// Получение возвращаемого значения, если оно есть
	eval_expression(&value);
	ret_value = value;
}
/**
 * @brief Затолкнуть локальную переменную в local_var_stack
 * @param local_variable
 */
void local_push(struct variable_type local_variable)
{
	if (local_var_to_stack >= NUM_LOCAL_VARS)
	{
		syntax_error(TOO_MANY_LVARS);
	}
	else
	{
		local_var_stack[local_var_to_stack] = local_variable;
		local_var_to_stack++;
	}
}
/**
 * @brief Выталкивание индекса в стеке локальных переменных
 * @return
 */
int func_pop()
{
	int index = 0;
	function_last_index_on_call_stack--;
	if (function_last_index_on_call_stack < 0)
	{
		syntax_error(RET_NOCALL);
	}
	else if (function_last_index_on_call_stack >= NUMBER_FUNCTIONS)
	{
		syntax_error(NESTED_FUNCTIONS);
	}
	else
	{
		index = call_stack[function_last_index_on_call_stack];
	}

	return index;
}
/**
 * @brief Запись индекса в стек локальных переменных
 * @param i
 */
void function_push_variables_on_call_stack(int i)
{
	if (function_last_index_on_call_stack >= NUMBER_FUNCTIONS)
	{
		syntax_error(NESTED_FUNCTIONS);
	}
	else
	{
		call_stack[function_last_index_on_call_stack] = i;
		function_last_index_on_call_stack++;
	}
}
/**
 * @brief Запись индекса в стек локальных переменных
 * @param var_name
 * @param value
 */
void assign_var(char *var_name, int value)
{
	int i;
	/// Проверка наличия локальной переменной
	for (i = local_var_to_stack - 1; i >= call_stack[function_last_index_on_call_stack - 1]; i--)
	{
		if (!strcmp(local_var_stack[i].variable_name, var_name))
		{
			local_var_stack[i].variable_value = value;
			return;
		}
	}
	/// Если переменная нелокальная, ищем ее в таблице глобальных переменных
	if (i < call_stack[function_last_index_on_call_stack - 1])
		for (i = 0; i < NUM_GLOBAL_VARS; i++)
			if (!strcmp(global_vars[i].variable_name, var_name))
			{
				global_vars[i].variable_value = value;
				return;
			}
	/// Переменной не существует
	syntax_error(NOT_VAR);
}
/**
 * @brief Получение значения переменной
 * @param s
 * @return
 */
int find_var(char *s)
{
	int i;
	/// Проверка наличия переменной
	for (i = local_var_to_stack - 1; i >= call_stack[function_last_index_on_call_stack - 1]; i--)
		if (!strcmp(local_var_stack[i].variable_name, current_token))
			return local_var_stack[i].variable_value;
	/// в противном случае проверим, может быть это глобальная переменная
	for (i = 0; i < NUM_GLOBAL_VARS; i++)
		if (!strcmp(global_vars[i].variable_name, s))
			return global_vars[i].variable_value;
	/// Переменной не существует
	syntax_error(NOT_VAR);
	return -1;
}
/**
 * @brief Проверка на переменную
 * @param s
 * @return Если является переменной, то возвращается 1, иначе 0.
 */
int is_variable(char *s)
{
	int i;
	/// Это локальная переменная ?
	for (i = local_var_to_stack - 1; i >= call_stack[function_last_index_on_call_stack - 1]; i--)
		if (!strcmp(local_var_stack[i].variable_name, current_token))
			return 1;
	/// Если нет - поиск среди глобальных переменных
	for (i = 0; i < NUM_GLOBAL_VARS; i++)
		if (!strcmp(global_vars[i].variable_name, s))
			return 1;
	return 0;
}
/**
 * @brief Выполнение оператора if
 */
void execute_if_statement()
{
	int condition;
	eval_expression(&condition); /* вычисление if-выражения */
	if (condition)				 /* истина - интерпретация if-предложения */
	{
		interpret_block();
	}
	else /* в противном случае пропуск if-предложения и выполнение else-предложения, если оно есть */
	{
		find_eob(); /* поиск конца блока */
		get_next_token();
		if (current_tok_datatype != ELSE)
		{
			shift_source_code_location_back(); /* восстановление лексемы, если else-предложение отсутствует */
			return;
		}
		interpret_block();
	}
}
/**
 * @brief Выполнение блока while
 */
void exec_while()
{
	int condition;
	char *temp;
	break_occurring = 0; /* флаг break */
	shift_source_code_location_back();
	temp = source_code_location; /* запоминание адреса начала цикла while */
	get_next_token();
	eval_expression(&condition); /* вычисление управляющего выражения */
	if (condition)				 /* если оно истинно, то выполнить интерпретацию */
	{
		interpret_block();
		if (break_occurring > 0)
		{
			break_occurring = 0;
			return;
		}
	}
	else /* в противном случае цикл пропускается */
	{
		find_eob();
		return;
	}
	source_code_location = temp; /* возврат к началу цикла */
}
/**
 * @brief Выполнение блока do
 */
void exec_do()
{
	int condition;
	char *temp;

	shift_source_code_location_back();
	temp = source_code_location; /* запоминание адреса начала цикла */
	break_occurring = 0;		 /* флаг break */

	get_next_token();  /* найти начало цикла */
	interpret_block(); /* интерпритация цикла */
	if (ret_occurring > 0)
	{
		return;
	}
	else if (break_occurring > 0)
	{
		break_occurring = 0;
		return;
	}
	get_next_token();
	if (current_tok_datatype != WHILE)
		syntax_error(WHILE_EXPECTED);
	eval_expression(&condition); /* проверка условия цикла */
	if (condition)
		source_code_location = temp; /* если условие истинно, то цикл выполняется, в противном случае происходит выход из цикла */
}
/**
 * @brief Поиск конца блока
 */
void find_eob(void)
{
	int brace;

	get_next_token();
	brace = 1;
	do
	{
		get_next_token();
		if (*current_token == '{')
			brace++;
		else if (*current_token == '}')
			brace--;
	} while (brace);
}
/**
 * @brief Выполнение блока for
 */
void exec_for(void)
{
	int condition;
	char *temp, *temp2;
	int brace;

	break_occurring = 0; /* флаг break */
	get_next_token();
	eval_expression(&condition); /* инициализирующее выражение */
	if (*current_token != ';')
		syntax_error(SEMICOLON_EXPECTED);

	source_code_location++; /* пропуск ; */
	temp = source_code_location;
	for (;;)
	{
		eval_expression(&condition); /* проверка условия */
		if (*current_token != ';')
			syntax_error(SEMICOLON_EXPECTED);
		source_code_location++; /* пропуск ; */
		temp2 = source_code_location;

		/// Поиск начала тела цикла
		brace = 1;
		while (brace)
		{
			get_next_token();
			if (*current_token == '(')
				brace++;
			if (*current_token == ')')
				brace--;
		}

		/// если условие выполнено, то выполнить интерпретацию */
		if (condition)
		{
			interpret_block();
			if (ret_occurring > 0)
			{
				return;
			}
			else if (break_occurring > 0)
			{
				break_occurring = 0;
				return;
			}
		}
		else /* в противном случае обойти цикл */
		{
			find_eob();
			return;
		}
		source_code_location = temp2;
		eval_expression(&condition); /* выполнение инкремента */
		source_code_location = temp; /* возврат в начало цикла */
	}
}
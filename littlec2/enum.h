/**
 * @brief Типы токенов
 */
enum token_types
{
	DELIMITER, //Разделители по типу +-=!? и т.д.
	VARIABLE,  //Переменная
	NUMBER,	   //Число
	KEYWORD,   //Ключевое слов
	TEMP,	   //Буфер
	STRING,	   //Строка
	BLOCK	   //Блок кода с функцией
};
/**
 * @brief Ключевые слова
 */
enum tokens
{
	ARG,	  //Аргумент
	CHAR,	  //Вещественный тип данных
	INT,	  //Целочисленный тип данных
	IF,		  //Оператор if
	ELSE,	  //Оператор else
	FOR,	  //Оператор for
	DO,		  //Оператор do
	WHILE,	  //Оператор while
	SWITCH,	  //Оператор switch
	RETURN,	  //Оператор return
	CONTINUE, //Оператор continue
	BREAK,	  //Оператор break
	EOL,	  //Конец строки
	FINISHED,
	END
};
/**
 * @brief Операторы отношений
 */
enum double_ops
{
	LOWER = 1, //Меньше, меньше или равно
	LOWER_OR_EQUAL,
	GREATER, //Больше, больше или равно
	GREATER_OR_EQUAL,
	EQUAL, //Эквивалентно или не эквивалентно
	NOT_EQUAL
};
/**
 * @brief Виды ошибок
 */
enum error_msg
{
	SYNTAX,				//синтаксическая ошибка
	UNBAL_PARENS,		//Скобок слишком много или не хватает
	NO_EXP,				//Ожидалось выражение
	EQUALS_EXPECTED,	//Ожидалось приравнивание
	NOT_VAR,			//не переменная
	PARAM_ERR,			//неправильно задан параметр
	SEMICOLON_EXPECTED, //ожидалась ;
	UNBAL_BRACES,		//операторных скобок много или не хватает
	FUNC_UNDEFINED,		//функция определена неправильно
	TYPE_EXPECTED,		//тип данных не указан
	NESTED_FUNCTIONS,	//функция определена внутри другой функции
	RET_NOCALL,			//функция ничего не возвращает
	PAREN_EXPECTED,		//ожидалась скобка
	WHILE_EXPECTED,		//ожидался while
	QUOTE_EXPECTED,		//неправильно написан комментарий
	NOT_TEMP,			//не строка
	TOO_MANY_LVARS,		//слишком много локальных переменных
	DIV_BY_ZERO			//на ноль делить нельзя
};
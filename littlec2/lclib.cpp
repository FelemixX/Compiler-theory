/**
 * Библиотека функций Little C
 */

#if defined(_MSC_VER)
#include <conio.h> /* Если комплиятор ругается на это,
 *                  То можно убрать */
#endif

#include <stdio.h>
#include <stdlib.h>
#include "enum.h"

extern char *source_code_location; /* текущее положение в исходном тексте программы */
extern char current_token[80];	   /* строковое представление current_token */
extern char token_type;			   /* содержит тип current_token */
extern char current_tok_datatype;  /* внутреннее представление current_token */

int get_next_token(void);
void syntax_error(int error), eval_expression(int *result);
void shift_source_code_location_back(void);
/**
 * Считывание символа с консоли. Если компилятор не поддерживает _getche(), то следует использвать getchar()
 * @return
 */
int call_getche(void)
{
	char ch;
#if defined(_QC)
	ch = (char)getche();
#elif defined(_MSC_VER)
	ch = (char)_getche();
#else
	ch = (char)getchar();
#endif
	while (*source_code_location != ')')
		source_code_location++;
	/// Продолжаем работать, пока не достигнем конца строки
	source_code_location++;
	return ch;
}
/**
 * Вывод символа на экран
 * @return
 */
int call_putch(void)
{
	int value;

	eval_expression(&value);
	printf("%c", value);
	return value;
}
/**
 * Вызов функции puts()
 * @return
 */
int call_puts(void)
{
	get_next_token();
	if (*current_token != '(')
		syntax_error(PAREN_EXPECTED);
	get_next_token();
	if (token_type != STRING)
		syntax_error(QUOTE_EXPECTED);
	puts(current_token);
	get_next_token();
	if (*current_token != ')')
		syntax_error(PAREN_EXPECTED);

	get_next_token();
	if (*current_token != ';')
		syntax_error(SEMICOLON_EXPECTED);
	shift_source_code_location_back();
	return 0;
}
/**
 * Встроенная функция консольного вывода
 * @return
 */
int print(void)
{
	int i;

	get_next_token();
	if (*current_token != '(')
		syntax_error(PAREN_EXPECTED);

	get_next_token();
	if (token_type == STRING)
	{ /* выводим строку */
		printf("%s ", current_token);
	}
	else
	{ /* выводим число */
		shift_source_code_location_back();
		eval_expression(&i);
		printf("%d ", i);
	}

	get_next_token();

	if (*current_token != ')')
		syntax_error(PAREN_EXPECTED);

	get_next_token();
	if (*current_token != ';')
		syntax_error(SEMICOLON_EXPECTED);
	shift_source_code_location_back();
	return 0;
}
/**
 * Считывание целого числа с клавиатуры
 * @return
 */
int getnum(void)
{
	char s[80];

	if (fgets(s, sizeof(s), stdin) != NULL)
	{
		while (*source_code_location != ')')
			source_code_location++;
		source_code_location++; /* читаем до конца строки */
		return atoi(s);
	}
	else
	{
		return 0;
	}
}
#input = "02232*+*2*-"
# sqrt(-3+2*2)*7+1*(1+5*4)*(3+5*6+2)*sqrt(1+4*5+1)
#exp = "003-22*+sqrt7*1154*+*356*+2+*0145*+1+sqrt*+"
# 3+sqrt(5)*6/6+sqrt(7)
#exp = "305sqrt6*6/+07sqrt+"
#input = "204sqrt9*9/+02sqrt+"
# ((0-(sqrt(((0-3)+(2*2)))*7))+(((1*(1+(5*4)))*((3+(5*6))+2))*sqrt(((1+(4*5))+1))))
#input = "003-22*+sqrt7*-1154*+*356*+2+*0145*+1+sqrt*+"

def postfix_to_infix(expression):
    stack = []
    i = 0
    while i < len(expression):
        to_convert = expression[i]

        if expression[i:i+4] == "sqrt":
            stack_a = stack.pop(0)
            stack_b = stack.pop(0)
            stack.insert(0, f"sqrt({stack_a})")
            i += 4

        elif to_convert.isdigit():
            stack.insert(0, to_convert)
            i += 1
        else:
            stack_a = stack.pop(0)
            stack_b = stack.pop(0)
            stack.insert(0, f"({stack_b}{to_convert}{stack_a})")
            i += 1

    return stack[0]

def remove_brackets(term):
    a = 0
    while True:
        #Ищем открывающие скобки
        try:
            a = term.index("(", a)
        except ValueError:
            #Открывающие скобки кончились
            break
            #Ищем закрывающие собки
        b = a
        while True:
            b = term.index(")", b + 1)
            if term[a + 1:b].count("(") == term[a + 1:b].count(")"):
                break
            #Собираем выражение уже без лишних скобок
        new_term = term[:a] + term[a + 1:b] + term[b + 1:]
        #Если смысл выражения без удаленных скобок поменялся, то оставляем скобки и пробуем еще раз
        if eval(term) != eval(new_term):
            a += 1
            continue
        #Записываем новое выражение
        term = new_term
    return term

standard_notation = postfix_to_infix(input)
print(standard_notation)
#fixed_brackets = remove_brackets(standard_notation)
#print(fixed_brackets)

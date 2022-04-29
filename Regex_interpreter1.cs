//reg (bc|c)*(bd*)+
//cbbb bdddbbbd
string expression = "cbccbcdbb dbdddbbbdddd"; //строка, которая идет на вход
string tmp1 = " "; //первая часть регулярки (до третьей скобки)
string tmp2 = " ";
string result = " "; //результирующая строка, которую запихнем в список
List<string> matches = new List<string>(); //список для удобного хранения результата работы

if (expression.Length == 0)
    Console.WriteLine("EOL");

for (int i = 0; i < expression.Length; i++)
{
    if (expression[i] == ' ')
        continue;
    ;
    if ((expression[i] == 'b' && expression[i + 1] == 'c') || expression[i] == 'c')
    {
        if (expression [i] == 'b' && expression [i + 1] == 'c')
        {
            tmp1 += expression[i];
            tmp1 += expression[i + 1];
            i += 2;
        }
        if (expression[i] == 'c')
        {
            tmp1 += expression[i];
            i++;
        }
        if (i >= expression.Length) 
            break;
    }

    if (tmp1.Length > 0 && (expression[i] == 'b' && expression[i + 1] == 'd'))
    {
        while (expression[i] == 'b' && expression[i + 1] == 'd')
        { 
            int outOfBoundCheck = 0; //костыль, чтобы не попасть в петлю и не вылететь за границу массива
            tmp2 += expression[i];
            tmp2 += expression[i + 1];
            i += 2;
            outOfBoundCheck = i + 1;
            if (outOfBoundCheck >= expression.Length)
                break;
        }
    }
    
}

result = tmp1 + tmp2;

if (result.Length > 0)
    matches.Add(result);
foreach(var match in matches)
    Console.WriteLine(match);
    

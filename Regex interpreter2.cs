            string expression = "cabcas\\dfgc baaa+-aaaab *ababcddfdcbddd..\\dbbbccdcdcdbbbbd@";
            //Console.WriteLine("Input your expression => ");
           // string expression = Console.ReadLine();
            string tmp1 = " ";  string tmp2 = " "; //без табуляции почему-то не запускается
            string result = " ";
            List<string> matches = new List<string>();
            if (expression.Length == 0)
                Console.WriteLine("Expression is empty");
            for (int i = 0; i < expression.Length; i++)
            {
                if (expression[i] == ' ') //пропуск пустого вхождения (рудимент?)
                    i++;
                
                if ((expression[i] == 'a' || expression[i] == 'b' || expression[i] == 'c')) //поиск первой скобки
                {
                    tmp1 += expression[i]; 
                }
                
                if (tmp1.Length > 0  && (expression[i] == 'b' || (expression[i] == 'c' && expression[i + 1] == 'd')))
                {
                    while (expression[i] == 'b' || (expression[i] == 'c' && expression[i + 1] == 'd')) //поиск второй скобки и плюсика после нее
                    {
                        var check = 0; //костыль от бесконечной петли 
                        if (expression[i] == 'b') //отдельный поиск b из-за наличия плюсика
                        {
                            tmp2 += expression[i];
                            i++;
                            check = i + 1; //переводим каретку на один шаг вперед
                        }
                        else
                        {
                            tmp2 += expression[i]; //если не найдено b, ищем остальные символы 
                            tmp2 += expression[i + 1];
                            i += 2;
                            check = i + 1;
                        }

                        if (check >= expression.Length)
                            break;
                    }
                }

                if (i >= expression.Length)
                    break;
            }
            // if (tmp1.Length > 0 && tmp2.Length > 0)
           result = tmp1 + tmp2; //делаем конкатенацию двух буферных переменных
           if (result.Length > 0)
                matches.Add(result); //добавляем результат в список и выводим
            foreach (var match in matches) //можно выводить и без списка
            {
                Console.WriteLine($"Regex result => {match}");
            }

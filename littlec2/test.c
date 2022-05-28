int testFunc(int num)
{
    int sum, i;
    sum = 0;
    print(0);
    for (i = 1; i < num; i = i + 1)
    {
        print("+");
        print(i);
        sum = sum + i;
    }
    print("=");
    print(sum);
    puts("");

    return 0;
}

int doWhile(int test)
{
    int test2;
    test = 3;
    do
    {
        test = test - 1;
        print("\nDoWhile is working now...\n");
        test2 = test;
    } while (test2);
    return 0;
}

int catchMeBitch (int catch)
{
    print("\nCatch is:\n");
    print(catch);
    return 0;
}

int throwMeBitch(int i)
{
    int catch;
    while (catch <=2)
    {
        i = i + 1;
        catch = i;
        catchMeBitch(catch);
    }
    return 0;
}

int main()
{
    int test;
    int l;
    l = 12;
    test = 12;
    int testIf;
    testIf = 20;
    int i;
    i = 0;
    puts("\ncheck\n");
    puts("pepepopochechk\n");

    testFunc(l);
    doWhile(test);
    throwMeBitch(i);
    print("Test is:"); // test
    print(test);

    while (test <= 15)
    {
        print("\nTesting while\n");
        print(test);
        test = test + 1;
    }
    if (i <= 12)
    {
        if (i == 10)
        {
            break;
        }
        testIf = testIf - 5;
        print("\ntestIf equals\n");
        print(testIf);
        i = i - 1;
    }
    return 0; /*test*/
}
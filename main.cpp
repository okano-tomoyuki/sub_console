#include "sub_console.hpp"

int main()
{
    SubConsole sub1("Test1");
    SubConsole sub2("Test2");

    for (int i = 0; i < 50; i++)
    {
        sub1.write("\e[31mHello,World.");
        sub2.write("\e[32mHello,World.");
        
        if (i % 10 == 0)
            sub1.clear();
        Sleep(100);
    }

    return 0;
}
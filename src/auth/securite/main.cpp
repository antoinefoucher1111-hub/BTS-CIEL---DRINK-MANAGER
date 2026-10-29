#include <iostream>
#include "securite.h"

int main()
{
    for (int i = 0; i < 10; ++i)
    {
        std::string code = Securite::genererCode();
        std::cout << code << "  (longueur " << code.size() << ")" << std::endl;
    }
    return 0;
}
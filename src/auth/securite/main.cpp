#include <iostream>
#include "securite.h"

int main()
{
    std::cout << Securite::hasher("abc") << std::endl;
    std::cout << Securite::hasher("abd") << std::endl;
    std::cout << Securite::hasher("abc") << std::endl;
    return 0;
}
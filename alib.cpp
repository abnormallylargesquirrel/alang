#include <iostream>
#include <limits>
#include "alib.h"

void printInt(std::int64_t n)
{
    //printf("%" PRId64 "", n);
    std::cout << n << std::endl;
}

void printDbl(double n)
{
    static bool first = true;
    if(first) {
        std::cout.precision(std::numeric_limits<double>::digits10);
        first = false;
    }
    std::cout << n << std::endl;
}

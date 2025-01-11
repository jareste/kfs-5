#include "utils.h"

uint32_t strtol(const char* str, char** endptr, int base)
{
    uint32_t result = 0;
    int i = 0;
    while (str[i] == ' ')
    {
        i++;
    }
    bool negative = false;
    if (str[i] == '-')
    {
        negative = true;
        i++;
    }
    while (str[i] == ' ')
    {
        i++;
    }
    while (str[i] >= '0' && str[i] <= '9')
    {
        result = result * base + str[i] - '0';
        i++;
    }
    if (endptr)
    {
        *endptr = (char*)&str[i];
    }
    return negative ? -result : result;
}

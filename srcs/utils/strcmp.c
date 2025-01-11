#include "utils.h"
bool strcmp(const char *str1, const char *str2)
{
    int i = 0;
    while (str1[i] && str2[i])
    {
        if (str1[i] != str2[i])
            return false;
        i++;
    }
    if (str1[i] || str2[i])
        return false;
    return true;
}

bool strncmp(const char *str1, const char *str2, int n)
{
    int i = 0;
    while (str1[i] && str2[i] && i < n)
    {
        if (str1[i] != str2[i])
            return false;
        i++;
    }
    if (i == n)
        return true;
    if (str1[i] || str2[i])
        return false;
    return true;
}

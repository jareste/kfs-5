#include "utils.h"
char *strtok(char *str, const char *delim)
{
    static char *last = NULL;
    if (str == NULL && last == NULL)
    {
        return NULL;
    }
    if (str == NULL)
    {
        str = last;
    }
    str += strspn(str, delim);
    if (*str == '\0')
    {
        last = NULL;
        return NULL;
    }
    char *start = str;
    str += strcspn(str, delim);
    if (*str == '\0')
    {
        last = NULL;
    }
    else
    {
        *str = '\0';
        last = str + 1;
    }
    return start;
}
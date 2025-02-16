#include "stdint.h"
size_t strspn(const char *s, const char *accept)
{
    size_t i = 0;
    while (s[i])
    {
        size_t j = 0;
        while (accept[j])
        {
            if (s[i] == accept[j])
            {
                break;
            }
            j++;
        }
        if (!accept[j])
        {
            return i;
        }
        i++;
    }
    return i;
}

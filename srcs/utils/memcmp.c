#include "stdint.h"
int memcmp(const void* s1, const void* s2, size_t n)
{
    const uint64_t* p1 = (const uint64_t*)s1;
    const uint64_t* p2 = (const uint64_t*)s2;
    
    size_t chunks = n / 8;
    for (size_t i = 0; i < chunks; i++)
    {
        if (p1[i] != p2[i])
        {
            const uint8_t* b1 = (const uint8_t*)&p1[i];
            const uint8_t* b2 = (const uint8_t*)&p2[i];
            for (int j = 0; j < 8; j++)
            {
                if (b1[j] != b2[j])
                {
                    return b1[j] - b2[j];
                }
            }
        }
    }
    
    const uint8_t* p1_8 = (const uint8_t*)(p1 + chunks);
    const uint8_t* p2_8 = (const uint8_t*)(p2 + chunks);
    size_t remaining = n % 8;
    
    for (size_t i = 0; i < remaining; i++)
    {
        if (p1_8[i] != p2_8[i])
        {
            return p1_8[i] - p2_8[i];
        }
    }
    
    return 0;
}

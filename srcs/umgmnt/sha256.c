#include "../utils/stdint.h"
#include "../utils/utils.h"
#include "../memory/memory.h"

typedef struct
{
    uint32_t h[8];
} sha256_ctx_t;

static uint32_t rightrotate(uint32_t x, uint32_t c)
{
    return (x >> c) | (x << (32 - c));
}

static void sha256(const uint8_t *initial_msg, size_t initial_len, uint8_t *digest)
{
    static const uint32_t k[] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    uint8_t *msg = NULL;
    size_t new_len, offset;
    uint32_t w[64];
    uint32_t a, b, c, d, e, f, g, h, temp1, temp2;

    sha256_ctx_t ctx;

    ctx.h[0] = 0x6a09e667;
    ctx.h[1] = 0xbb67ae85;
    ctx.h[2] = 0x3c6ef372;
    ctx.h[3] = 0xa54ff53a;
    ctx.h[4] = 0x510e527f;
    ctx.h[5] = 0x9b05688c;
    ctx.h[6] = 0x1f83d9ab;
    ctx.h[7] = 0x5be0cd19;

    new_len = initial_len + 1;
    while (new_len % 64 != 56)
    {
        new_len++;
    }

    msg = kmalloc(new_len + 8);
    memset(msg, 0, new_len + 8);
    memcpy(msg, initial_msg, initial_len);
    msg[initial_len] = 0x80;

    uint64_t bits_len = initial_len * 8;
    for (int i = 0; i < 8; i++)
    {
        msg[new_len + i] = (bits_len >> (56 - 8 * i)) & 0xff;
    }

    for (offset = 0; offset < new_len; offset += 64)
    {
        for (int i = 0; i < 16; i++)
        {
            w[i] = (uint32_t) msg[offset + i * 4] << 24 | (uint32_t) msg[offset + i * 4 + 1] << 16 |
                   (uint32_t) msg[offset + i * 4 + 2] << 8 | (uint32_t) msg[offset + i * 4 + 3];
        }

        for (int i = 16; i < 64; i++)
        {
            uint32_t s0 = rightrotate(w[i - 15], 7) ^ rightrotate(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = rightrotate(w[i - 2], 17) ^ rightrotate(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        a = ctx.h[0];
        b = ctx.h[1];
        c = ctx.h[2];
        d = ctx.h[3];
        e = ctx.h[4];
        f = ctx.h[5];
        g = ctx.h[6];
        h = ctx.h[7];

        for (int i = 0; i < 64; i++)
        {
            uint32_t s1 = rightrotate(e, 6) ^ rightrotate(e, 11) ^ rightrotate(e, 25);
            uint32_t ch = (e & f) ^ (~e & g);
            temp1 = h + s1 + ch + k[i] + w[i];
            uint32_t s0 = rightrotate(a, 2) ^ rightrotate(a, 13) ^ rightrotate(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            temp2 = s0 + maj;

            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        ctx.h[0] += a;
        ctx.h[1] += b;
        ctx.h[2] += c;
        ctx.h[3] += d;
        ctx.h[4] += e;
        ctx.h[5] += f;
        ctx.h[6] += g;
        ctx.h[7] += h;
    }

    for (int i = 0; i < 8; i++)
    {
        digest[i * 4] = (ctx.h[i] >> 24) & 0xff;
        digest[i * 4 + 1] = (ctx.h[i] >> 16) & 0xff;
        digest[i * 4 + 2] = (ctx.h[i] >> 8) & 0xff;
        digest[i * 4 + 3] = ctx.h[i] & 0xff;
    }

    kfree(msg);
}

void byte_to_hex(uint8_t byte, char *hex)
{
    const char hex_chars[] = "0123456789abcdef";
    hex[0] = hex_chars[(byte >> 4) & 0x0F];
    hex[1] = hex_chars[byte & 0x0F];
}

int encrypt_password(const char *password, char *encrypt)
{
    uint8_t digest[32];
    sha256((const uint8_t *)password, strlen(password), digest);
    
    for (int i = 0; i < 32; i++)
    {
        byte_to_hex(digest[i], encrypt + i * 2);
    }
    encrypt[64] = '\0';

    return 0;
}

int check_password(const char *password, const char *encrypt)
{
    char encrypt_buf[65];
    encrypt_password(password, encrypt_buf);
    printf("buf buf: %s\n", encrypt_buf);
    printf("encrypt: %s\n", encrypt);
    printf("strcmp: %d\n", strcmp(encrypt, encrypt_buf) == 0);
    return strcmp(encrypt, encrypt_buf) == 0;
}

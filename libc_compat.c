
// libc_compat.c

/*
 * Part of Penetrizzel
 * Copyright (c) 2026 ArChanum
 */

#include "libc_compat.h"
#include <stddef.h>
#include <stdarg.h>


//BEGIN FISR
float Q_rsqrt( float number )
{
        uint32_t i;
        float x2, y;
        const float threehalfs = 1.5f;

        x2 = number * 0.5f;
        y  = number;
      //i = * ( long * ) &y;                    // evil floating


                                                union
                                                {
                                                    float f;
                                                    uint32_t i;

                                                } conv;

                                                conv.f = y;
                                                i = conv.i;

        //point bit level hacking
        i = 0x5F3759DFu - (i >> 1);             // what the f☀⛤k?


                                                conv.i = i;
                                                y = conv.f;


      //y = * ( float * ) &i;
        y = y * (threehalfs - (x2 * y * y));        // 1st iteration
//      y = y * (threehalfs - (x2 * y * y));        // 2nd iteration,
        //this can be removed

    return y;
}

float Q_sqrt(float number)
{
    if (number <= 0.0f) return 0.0f;

    return number * Q_rsqrt(number);
}
//END FISR


void *memset(void *dst, int value, size_t count)
{
    unsigned char *d = dst;

    while(count--)
    {
        *d++ = (unsigned char)value;
    }

    return dst;
}

void *memcpy(void *dst, const void *src, size_t count)
{
    unsigned char *d = dst;
    const unsigned char *s = src;

    while(count--)
    {
        *d++ = *s++;
    }

    return dst;
}

void *memmove(void *dst, const void *src, size_t count)
{
    unsigned char *d;
    const unsigned char *s;

    if (dst == src || count == 0)   return dst;

    d = (unsigned char *)dst;
    s = (const unsigned char *)src;

    if (d < s)
    {
        while (count--)
        {
            *d++ = *s++;
        }
    }
    else
    {
        d += count;
        s += count;

        while (count--)
        {
            *--d = *--s;
        }
    }

    return dst;
}

int memcmp(const void *a, const void *b, size_t count)
{
    const unsigned char *x=a;
    const unsigned char *y=b;

    while(count--)
    {
        if(*x != *y)    return *x - *y;

        x++;
        y++;
    }

    return 0;
}

size_t strlen(const char *s)
{
    const char *p = s;

    while (*p)
        p++;

    return (size_t)(p - s);
}

char *strncpy(char *dst, const char *src, size_t count)
{
    size_t i;

    if (!dst || !src || count == 0) return dst;

    for (i = 0; i < count; ++i)
    {
        dst[i] = src[i];

        if (src[i] == '\0')
        {
            // C strncpy semantics: zero-fill the remainder.
            for (++i; i < count; ++i)
                dst[i] = '\0';

            break;
        }
    }

    return dst;
}

int strcmp(const char *a, const char *b)
{
    if (a == b) return 0;
    if (!a)     return -1;
    if (!b)     return 1;

    while (*a && *a == *b)
    {
        ++a;
        ++b;
    }

    return (unsigned char)*a - (unsigned char)*b;
}

int string_to_int(const char *s, bool *ok)
{
    int sign = 1;
    int value = 0;
    bool valid = false;

    if (ok) *ok = false;
    if (!s)     return 0;

    while (*s == ' ' || *s == '\t')
        ++s;

    if (*s == '-')
    {
        sign = -1;
        ++s;
    }
    else if (*s == '+')
    {
        ++s;
    }

    while (*s >= '0' && *s <= '9')
    {
        value = value * 10 + (*s - '0');
        valid = true;
        ++s;
    }

    if (ok) *ok = valid;

    return value * sign;
}

static void libc_compat_append_char(char *dst, size_t dst_size, size_t *pos, char c)
{
    if (*pos + 1 < dst_size)    dst[*pos] = c;

    (*pos)++;
}


static void libc_compat_append_string(char *dst, size_t dst_size, size_t *pos, const char *s)
{
    if (!s) s = "(null)";

    while (*s)
    {
        libc_compat_append_char(dst, dst_size, pos, *s);

        ++s;
    }
}


static void libc_compat_append_unsigned(char *dst, size_t dst_size, size_t *pos, unsigned int value, unsigned int base, bool uppercase)
{
    char buffer[32];
    int count = 0;

    if (value == 0)
    {
        libc_compat_append_char(dst, dst_size, pos, '0');

        return;
    }

    while (value != 0)
    {
        unsigned int digit = value % base;

        if (digit < 10)
        {
            buffer[count++] = (char)('0' + digit);
        }
        else if (uppercase)
        {
            buffer[count++] = (char)('A' + digit - 10);
        }
        else
        {
            buffer[count++] = (char)('a' + digit - 10);
        }

        value /= base;
    }

    while (count > 0)
    {
        --count;

        libc_compat_append_char(dst, dst_size, pos, buffer[count]);
    }
}


static void libc_compat_append_signed(char *dst, size_t dst_size, size_t *pos, int value)
{
    unsigned int magnitude;

    if (value < 0)
    {
        libc_compat_append_char(dst, dst_size, pos, '-');

        // Avoid signed overflow for INT_MIN.
        magnitude = (unsigned int)(-(value + 1)) + 1u;
    }
    else
    {
        magnitude = (unsigned int)value;
    }

    libc_compat_append_unsigned(dst, dst_size, pos, magnitude, 10u, false);
}


int snprintf(char *dst, size_t dst_size, const char *format, ...)
{
    va_list args;
    size_t pos = 0;

    if (!dst && dst_size != 0)  return -1;

    if (!format)
    {
        if (dst_size != 0)
            dst[0] = '\0';

        return -1;
    }

    va_start(args, format);

    while (*format)
    {
        if (*format != '%')
        {
            libc_compat_append_char(dst, dst_size, &pos, *format);

            ++format;
            continue;
        }

        ++format;

        if (*format == '\0')
            break;

        switch (*format)
        {
            case '%':
                libc_compat_append_char(dst, dst_size, &pos, '%');
                break;

            case 'c':
            {
                int value = va_arg(args, int);

                libc_compat_append_char(dst, dst_size, &pos, (char)value);

                break;
            }

            case 's':
            {
                const char *value = va_arg(args, const char *);

                libc_compat_append_string(dst, dst_size, &pos, value);

                break;
            }

            case 'd':
            case 'i':
            {
                int value = va_arg(args, int);

                libc_compat_append_signed(dst, dst_size, &pos, value);

                break;
            }

            case 'u':
            {
                unsigned int value = va_arg(args, unsigned int);

                libc_compat_append_unsigned(dst, dst_size, &pos, value, 10u, false);

                break;
            }

            case 'x':
            {
                unsigned int value = va_arg(args, unsigned int);

                libc_compat_append_unsigned(dst, dst_size, &pos, value, 16u, false);

                break;
            }

            case 'X':
            {
                unsigned int value = va_arg(args, unsigned int);

                libc_compat_append_unsigned(dst, dst_size, &pos, value, 16u, true);

                break;
            }

            default:
                // Unknown format. Keep the '%' so an unsupported conversion is visible rather than silently disappearing.
                libc_compat_append_char(dst, dst_size, &pos, '%');

                libc_compat_append_char(dst, dst_size, &pos, *format);

                break;
        }

        ++format;
    }

    va_end(args);

    // snprintf() always NUL terminates when dst_size > 0.
    if (dst_size != 0)
    {
        size_t terminator = pos;

        if (terminator >= dst_size)
            terminator = dst_size - 1;

        dst[terminator] = '\0';
    }

    // Like real snprintf(), return the number of characters that would have been written, excluding the terminating NULL.
    if (pos > (size_t)0x7FFFFFFF)
        return 0x7FFFFFFF;

    return (int)pos;
}

// HACK needed for clownresampler.h
static double clown_math_reduce_pi(double value)
{
    const double two_pi = 6.283185307179586476925286766559;

    const double pi = 3.141592653589793238462643383279502;

    while (value > pi)
        value -= two_pi;

    while (value < -pi)
        value += two_pi;

    return value;
}

double sin(double value)
{
    double x;
    double x2;

    x = clown_math_reduce_pi(value);

    // Range reduction into [-pi/2, pi/2].
    if (x > 1.57079632679489661923)
        x = 3.14159265358979323846 - x;
    else if (x < -1.57079632679489661923)
        x = -3.14159265358979323846 - x;

    x2 = x * x;

    // Taylor/minimax-style polynomial. More than adequate for resampler coefficient generation...
    return
        x *
        (1.0 -
        x2 *
        (1.0 / 6.0 -
        x2 *
        (1.0 / 120.0 -
        x2 *
        (1.0 / 5040.0 -
        x2 *
        (1.0 / 362880.0 -
        x2 *
        (1.0 / 39916800.0))))));
}

double cos(double value)
{
    return sin(value + 1.57079632679489661923);
}

//used for GLASS_shader
float absf(float x)
{
    return (x < 0.0f) ? -x : x;
}

float minf(float a, float b)
{
    return (a < b) ? a : b;
}

float maxf(float a, float b)
{
    return (a > b) ? a : b;
}

int32_t truncf_int(float x)
{
    return (int32_t)x;
}

int32_t floorf_int(float x)
{
    int32_t i = truncf_int(x);

    if ((float)i > x)
        --i;

    return i;
}

int32_t ceilf_int(float x)
{
    int32_t i = truncf_int(x);

    if ((float)i < x)
        ++i;

    return i;
}


#define LIBC_TRIG_TABLE_BITS 10
#define LIBC_TRIG_TABLE_SIZE (1u << LIBC_TRIG_TABLE_BITS)
#define LIBC_TRIG_TABLE_MASK (LIBC_TRIG_TABLE_SIZE - 1u)

#define LIBC_TWO_PI 6.28318530717958647692f // TODO use it everywhere...

static float libc_sin_table[LIBC_TRIG_TABLE_SIZE];

static bool libc_trig_initialized = false;

void libc_compat_trig_init(void)
{
    uint32_t i;

    if (libc_trig_initialized)
        return;

    for (i = 0; i < LIBC_TRIG_TABLE_SIZE; ++i)
    {
        double angle = (double)i * (6.28318530717958647692 / (double)LIBC_TRIG_TABLE_SIZE);

        libc_sin_table[i] = (float)sin(angle);
    }

    libc_trig_initialized = true;
}

float sinf(float value)
{
    uint32_t index;

    if (!libc_trig_initialized) libc_compat_trig_init();

    // Normalize to [0, 2PI).
    while (value < 0.0f)
        value += LIBC_TWO_PI;

    while (value >= LIBC_TWO_PI)
        value -= LIBC_TWO_PI;

    index = (uint32_t)(value * ((float)LIBC_TRIG_TABLE_SIZE / LIBC_TWO_PI));

    return libc_sin_table[index & LIBC_TRIG_TABLE_MASK];
}

float cosf(float value)
{
    return sinf(value + LIBC_TWO_PI * 0.25f);
}


// libc_compat.h

/*
 * Part of Penetrizzel
 * Copyright (c) 2026 ArChanum
 */

#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

// Prototypes
float Q_rsqrt( float number );
float Q_sqrt(float number);

void *memset(void *dst, int value, size_t count);
void *memcpy(void *dst, const void *src, size_t count);
void *memmove(void *dst, const void *src, size_t count);
int memcmp(const void *a, const void *b, size_t count);

size_t strlen(const char *s);
char *strncpy(char *dst, const char *src, size_t count);
int strcmp(const char *a, const char *b);
int string_to_int(const char *s, bool *ok);
int snprintf(char *dst, size_t dst_size, const char *format, ...);

double sin(double value);
double cos(double value);

float absf(float x);
float minf(float a, float b);
float maxf(float a, float b);

int32_t truncf_int(float x);
int32_t floorf_int(float x);
int32_t ceilf_int(float x);

void libc_compat_trig_init(void);

float sinf(float value);
float cosf(float value);

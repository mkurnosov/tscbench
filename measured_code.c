/*
 * measured_code.c:
 *
 * Copyright (C) Mikhail Kurnosov 2014 <mkurnosov@gmail.com>
 */

#include <math.h>
#include "measured_code.h"
 
#ifdef __GNUC__
#define NOOPTIMIZE __attribute__((optimize("O0")))
#else
#define NOOPTIMIZE
#endif

NOOPTIMIZE void empty() 
{
    /* Empty code */
}

int prime_numbers()
{
    int nprimes = 2;
    for (int n = 3; n < 1000; n += 2) {
        volatile int limit, factor = 3;
        limit = (long)(sqrt((float)n) + 0.5f);
        while ((factor <= limit) && (n % factor))
            factor++;
        if (factor > limit)
            nprimes++;
    }
    return nprimes;
}

#define SAXPY_N 1000
volatile float alpha = 3.14;
volatile float x[SAXPY_N], y[SAXPY_N];

float saxpy()
{   
    for (int i = 0; i < SAXPY_N; i++)
        y[i] = alpha * x[i] + y[i];
    return y[0];
}

#define DGEMM_N 512
volatile double a[DGEMM_N * DGEMM_N], b[DGEMM_N * DGEMM_N], c[DGEMM_N * DGEMM_N];

double dgemm()
{
    int i, j, k;
    
    for (i = 0; i < DGEMM_N; i++) {
        for (j = 0; j < DGEMM_N; j++) {
            *(c + i * DGEMM_N + j) = 0;
            for (k = 0; k < DGEMM_N; k++) {
                *(c + i * DGEMM_N + j) += *(a + i * DGEMM_N + k) * *(b + k * DGEMM_N + j);
            }
        }
    }
    return *c;
}

void loop_of_cpuid()
{
    for (int i = 0; i < 100; i++) {
        __asm__ __volatile__ (
            "xorl %%eax, %%eax\n"
            "cpuid\n"
            ::: "%rax", "%rbx", "%rcx", "%rdx"
        );
    }
}

void loop_of_mfence()
{
    for (int i = 0; i < 100; i++) {
        __asm__ __volatile__ (
            "mfence\n"
            :::
        );
    }
}

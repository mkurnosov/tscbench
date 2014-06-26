/*
 * tsc_x86.c: Routines for access to time-stamp counter on x86_64 processors.
 *
 * Copyright (C) Mikhail Kurnosov 2014 <mkurnosov@gmail.com>
 */
 
#include <stdio.h>
#include <inttypes.h>
#include "tsc_x86.h"
#include "mathstat.h"

/* is_tsc_available: Returns 1 if TSC subsystem is available. */
int is_tsc_available()
{
    uint32_t edx;
    __asm__ __volatile__ (
        "movl $0x1, %%eax\n"
        "cpuid\n"       
        "movl %%edx, %0\n"
        : "=r" (edx)
        :: "%rax", "%rbx", "%rcx", "%rdx"
    );    
    return edx & (1U << 4) ? 1 : 0;
} 

/* is_rdtscp_available: Returns 1 if RDTSCP instruction is supported. */
int is_rdtscp_available()
{
    uint32_t edx;
    __asm__ __volatile__ (
        "movl $0x80000001, %%eax\n"
        "cpuid\n"       
        "movl %%edx, %0\n"
        : "=r" (edx)
        :: "%rax", "%rbx", "%rcx", "%rdx"
    );
    return edx & (1U << 27) ? 1 : 0;
}

/* is_tsc_invariant: Returns 1 if TSC is invariant (constant rate + nonstop). */
int is_tsc_invariant()
{
    uint32_t edx;
    __asm__ __volatile__ (
        "movl $0x80000007, %%eax\n"
        "cpuid\n"       
        "movl %%edx, %0\n"
        : "=r" (edx)
        :: "%rax", "%rbx", "%rcx", "%rdx"
    );    
    return edx & (1U << 8) ? 1 : 0;
}    

/*
 * measure_tsc_overhead: Measures and returns minimal overhead for TSC reading.
 */
uint64_t measure_tsc_overhead()
{
    enum {
        NMEASURES = 100
    };    
    volatile uint64_t t0, t1, ticks, minticks = (uint64_t)~0x1;

    for (int i = 0; i < NMEASURES; ) {
        t0 = read_tsc_before();
        t1 = read_tsc_after();
        if (t1 > t0) {
            ticks = t1 - t0;
            if (ticks < minticks)
                minticks = ticks;
            i++;
        }
    }
    return minticks;
}

/*
 * measure_tsc_overhead_stabilized: Measures and returns minimal overhead
 *                                  for TSC reading. Stops measurements
 *                                  if results is not changed in the several
 *                                  recent launches. 
 */
uint64_t measure_tsc_overhead_stabilized()
{
    enum {
        NOTCHANGED_THRESHOLD = 10,
        NMEASURES_MAX = 100
    };    
    volatile uint64_t t0, t1, ticks, minticks = (uint64_t)~0x1;
    int notchanged = 0;
    
    for (int i = 0; i < NMEASURES_MAX && notchanged < NOTCHANGED_THRESHOLD; ) {
        t0 = read_tsc_before();
        t1 = read_tsc_after();
        if (t1 > t0) {
            ticks = t1 - t0;
            notchanged++;
            if (ticks < minticks) {
                minticks = ticks;
                notchanged = 0;
            }
            i++;
        }            
    }
    return minticks;
}

/* measure_tsc_overhead_rse: Measures overhead with given precision (RSE) */
uint64_t measure_tsc_overhead_rse()
{    
    #define RSE_MAX 5.0
    enum {
        NRUNS_MIN = 100,
        NRUNS_MAX = 1000000
    };
    
    stat_sample_t *stat = stat_sample_create();
    if (stat == NULL) {
        fprintf(stderr, "# No enough memory for statistics");
        return 0;
    }

    volatile uint64_t t0, t1;
    
    /* Warmup I-cache */
    for (int i = 0; i < 10; i++) {
        t0 = read_tsc_before();
        t1 = read_tsc_after();            
    }
    
    int nruns = NRUNS_MIN;
    do {
        stat_sample_clean(stat);
        for (int i = 0; i < nruns; ) {
            t0 = read_tsc_before();
            t1 = read_tsc_after();            
            /* Accumulate only correct results */
            if (t1 > t0) {
                stat_sample_add(stat, (double)(t1 - t0));
                i++;
            }
        }
        /* Reduce measurement error by increasing number of runs */
        nruns *= 4;        
    } while (stat_sample_size(stat) < NRUNS_MAX && stat_sample_rel_stderr_knuth(stat) > RSE_MAX);

    uint64_t overhead = stat_sample_mean(stat);
    printf("# Overhed measure RSE %.2f\n", stat_sample_rel_stderr(stat));
    stat_sample_free(stat);
    return overhead;
}

/*
 * normolize_ticks: Returns number of ticks between 2 reads of TSC (first & second)
 *                  minus overhead of TSC reading.
 */
uint64_t normolize_ticks(uint64_t first, uint64_t second, uint64_t tsc_overhead)
{
    if (second > first)
        return (second - first) > tsc_overhead ? second - first - tsc_overhead : 0;
    return 0;
}

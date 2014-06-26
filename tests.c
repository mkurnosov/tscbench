/*
 * tests.c: 
 *
 * Copyright (C) Mikhail Kurnosov 2014 <mkurnosov@gmail.com>
 */
#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "tsc_x86.h"

static inline void writetsc(uint64_t newtsc)
{
    /* Writing to TSC is only allowed in ring 0 */
    uint32_t ecx = 0x10; /* TSC */
    __asm__ __volatile__ (
        "wrmsr\n"                     
        :: "c" (ecx), "A" (newtsc)
    );
}

void prepare_system_for_benchmarking()
{
    if (geteuid() == 0) {
        /* Only ROOT can change scheduling policy to RT class */
        struct sched_param sp;
        memset(&sp, 0, sizeof(sp));
        sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
        if (sched_setscheduler(0, SCHED_FIFO, &sp) != 0)
            fprintf(stderr, "Error changing scheduling policy to RT class\n");
        else
            printf("# Scheduling policy is changed to RT class with max priority\n");
        
        /* Disable paging */
        if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0)
            fprintf(stderr, "Error locking pages\n");
        else 
            printf("# All pages of process are locked (paging is disabled)\n");
    } else {
        fprintf(stderr, "# Process is launched without ROOT permissions\n");
    }
}

static void show_tsc_info()
{        
    printf("# TSC subsystem available: %d\n", is_tsc_available() ? 1 : 0);
    printf("#   RDTSCP supported: %d\n", is_rdtscp_available() ? 1 : 0);
    printf("#   TSC invariant: %d\n", is_tsc_invariant() ? 1 : 0);

    printf("# RDTSC value: %" PRIu64 "\n", rdtsc());
    printf("# RDTSCP value: %" PRIu64 "\n", rdtscp());

    uint32_t aux;
    uint64_t tsc = rdtscp_aux(&aux);
    printf("# RDTSCP_AUX: TSC = %" PRIu64 ", IA32_TSC_AUX = %u\n", tsc, aux);

    /* Measure TSC overhead example */
    uint64_t overhead;
    overhead = measure_tsc_overhead();
    printf("# TSC overhead (ticks): %" PRIu64 "\n", overhead);

    overhead = measure_tsc_overhead_stabilized();
    printf("# TSC overhead stabilized (ticks): %" PRIu64 "\n", overhead);
}

void write_to_tsc()
{
    printf("# Trying to write TSC value 100...\n");
    uint32_t aux;
    uint64_t tsc = rdtscp_aux(&aux);
    printf("#   before write: RDTSCP_AUX: TSC = %" PRIu64 ", IA32_TSC_AUX = %u\n", tsc, aux);
    fflush(stdout);
    /* Assert: segmentation fault: general protection error -- we can do it only in ring 0 */
    writetsc(100);
    printf("#   after write: RDTSCP_AUX: TSC = %" PRIu64 ", IA32_TSC_AUX = %u\n", tsc, aux);
    fflush(stdout);
}

void check_migration()
{
    register uint32_t high0, low0, high1, low1, tscaux0, tscaux1;

    __asm__ __volatile__ (
        "xorl %%eax, %%eax\n"
        "cpuid\n"       
        "rdtscp\n"      
        "movl %%edx, %0\n"   
        "movl %%eax, %1\n"   
        "movl %%ecx, %2\n" /* IA32_TSC_AUX MSR */
        : "=r" (high0), "=r" (low0), "=r" (tscaux0)
        :: "%rax", "%rbx", "%rcx", "%rdx"
    );    
    
    /* Measured code */
        
    __asm__ __volatile__ (
        "rdtscp\n" 
        "movl %%edx, %0\n" 
        "movl %%eax, %1\n" 
        "movl %%ecx, %2\n" 
        "xorl %%eax, %%eax\n"            
        "cpuid\n"
        : "=r" (high1), "=r" (low1), "=r" (tscaux1)   
        :: "%rax", "%rbx", "%rcx", "%rdx"
    );

    printf("First RDTSCP: IA32_TSC_AUX = %" PRIu32 "\n", tscaux0);
    printf("Second RDTSCP: IA32_TSC_AUX = %" PRIu32 "\n", tscaux1);
    if (tscaux0 != tscaux1)
        fprintf(stderr, "Migration is occurred - second value of TSC was obtained from another CPU\n");
}

void check_migration_cpuid()
{
    register uint32_t high0, low0, high1, low1, tscaux0, tscaux1, cpuid0, cpuid1;

    __asm__ __volatile__ (
        "movl $0x0b, %%eax\n" /* Information about processor */
        "cpuid\n"       
        "movl %%edx, %3\n"    /* x2APIC ID the current logical processor */
        "rdtscp\n"      
        "movl %%edx, %0\n"   
        "movl %%eax, %1\n"    
        "movl %%ecx, %2\n"    /* IA32_TSC_AUX MSR */
        : "=r" (high0), "=r" (low0), "=r" (tscaux0), "=r" (cpuid0)
        :: "%rax", "%rbx", "%rcx", "%rdx"
    );    
        
    /* Migrate process to another processor */
    cpu_set_t set;
    sched_getaffinity(0, sizeof(set), &set);
    int ncpus = CPU_COUNT(&set);
    for (int cpu = 0; cpu < ncpus; cpu++)
        CPU_SET(cpu, &set);
    CPU_CLR(tscaux0, &set);
    
    if (sched_setaffinity(0, sizeof(set), &set) != 0)
        fprintf(stderr, "Error changing processor affinity\n");
    
    __asm__ __volatile__ (
        "rdtscp\n" 
        "movl %%edx, %0\n" 
        "movl %%eax, %1\n" 
        "movl %%ecx, %2\n"     /* IA32_TSC_AUX MSR */
        "movl $0x0b, %%eax\n"            
        "cpuid\n"
        "movl %%edx, %3\n"     /* x2APIC ID the current logical processor */
        : "=r" (high1), "=r" (low1), "=r" (tscaux1), "=r" (cpuid1)
        :: "%rax", "%rbx", "%rcx", "%rdx"
    );

    printf("Before code: IA32_TSC_AUX = %" PRIu32 "; CPU_ID = %" PRIu32 "\n", tscaux0, cpuid0);
    printf("After code:  IA32_TSC_AUX = %" PRIu32 "; CPU_ID = %" PRIu32 "\n", tscaux1, cpuid1);

    if (tscaux0 != tscaux1 || cpuid0 != cpuid1)
        fprintf(stderr, "Migration is occurred\n");            
}

int main()
{
    show_tsc_info();
    prepare_system_for_benchmarking();
    /* write_to_tsc(); */

    check_migration();
    check_migration_cpuid();
    
    return 0;
}

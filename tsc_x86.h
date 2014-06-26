/*
 * tsc_x86.h: Routines for access to time-stamp counter on x86_64 processors.
 *
 * Copyright (C) Mikhail Kurnosov 2014 <mkurnosov@gmail.com>
 */
 
#ifndef TSC_X86_H
#define TSC_X86_H

#include <inttypes.h>

#define TSC_READ_METHOD_STD

#if defined(TSC_READ_METHOD_INTEL)
    #define read_tsc_before read_tsc_before_intel
    #define read_tsc_after read_tsc_after_intel
#elif defined(TSC_READ_METHOD_LFENCE)
    #define read_tsc_before read_tsc_before_lfence
    #define read_tsc_after read_tsc_after_lfence
#elif defined(TSC_READ_METHOD_MFENCE)
    #define read_tsc_before read_tsc_before_mfence
    #define read_tsc_after read_tsc_after_mfence
#elif defined(TSC_READ_METHOD_CPUID4)
    #define read_tsc_before read_tsc_cpuid2
    #define read_tsc_after read_tsc_cpuid2
#else /* STD */
    #define read_tsc_before read_tsc_before_std 
    #define read_tsc_after read_tsc_after_std 
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* is_tsc_available: Returns 1 if TSC subsystem is available. */
int is_tsc_available();

/* is_rdtscp_available: Returns 1 if RDTSCP instruction is supported. */
int is_rdtscp_available();

/* is_tsc_invariant: Returns 1 if TSC is invariant (constant rate + nonstop). */
int is_tsc_invariant();

/*
 * measure_tsc_overhead: Measures and returns minimal overhead for TSC reading.
 */
uint64_t measure_tsc_overhead();

/*
 * measure_tsc_overhead_stabilized: Measures and returns minimal overhead
 * for TSC reading. Stops measurements if results is not changed in the several
 * recent launches. 
 */
uint64_t measure_tsc_overhead_stabilized();

/* measure_tsc_overhead_rse: Measures overhead with given precision (RSE) */
uint64_t measure_tsc_overhead_rse();

/*
 * normolize_ticks: Returns number of ticks between 2 reads of TSC (first & second)
 * minus overhead of TSC reading.
 */
uint64_t normolize_ticks(uint64_t first, uint64_t second, uint64_t tsc_overhead);

/* rdtsc: Reads and returns TSC value by RDTSC instruction. */
static inline uint64_t rdtsc()
{
    register uint32_t high, low;
    __asm__ __volatile__ (
        "rdtsc\n"                     
        "movl %%edx, %0\n"
        "movl %%eax, %1\n"
        : "=r" (high), "=r" (low)
        :: "%rax", "%rbx", "%rcx", "%rdx"
    );
    return ((uint64_t)high << 32) | low;
}

/* rdtscp: Reads TSC value by RDTSCP instruction. */
static inline uint64_t rdtscp()
{
    register uint32_t high, low;
    __asm__ __volatile__ (
        "rdtscp\n" 
        "movl %%edx, %0\n"
        "movl %%eax, %1\n"
        : "=r" (high), "=r" (low)
        :: "%rax", "%rbx", "%rcx", "%rdx"
    );    
    return ((uint64_t)high << 32) | low;
}

/*
 * rdtscp_aux: Reads and returns TSC value by RDTSCP,
 *             value of IA32_TSC_AUX MSR returns in *tsc_aux.
 */
static inline uint64_t rdtscp_aux(uint32_t *tsc_aux)
{
    register uint32_t high, low;
    __asm__ __volatile__ (
        "rdtscp\n" 
        "movl %%edx, %0\n"
        "movl %%eax, %1\n"
        "movl %%ecx, %2\n"
        : "=r" (high), "=r" (low), "=r" (*tsc_aux)
        :: "%rax", "%rbx", "%rcx", "%rdx"
    );    
    return ((uint64_t)high << 32) | low;
}

/* read_tsc_before_std: Standard approach (cpuid + rdtsc). */
static inline uint64_t read_tsc_before_std()
{
    register uint32_t high, low;
    /* 
     * 1. Prevent out-of-order execution (serializing by CPUID(0)):
     *    wait for the completion of all previous operations (before measured code)
     * 2. Read TSC value
     */
    __asm__ __volatile__ (
        "xorl %%eax, %%eax\n"
        "cpuid\n"                        /* Serialize execution */
        "rdtsc\n"                        /* Read TSC */
        "movl %%edx, %0\n"               /*   - high 32 bits */
        "movl %%eax, %1\n"               /*   - low 32 bits */
        : "=r" (high), "=r" (low)        /* Output */
        :                                /* Input */ 
        : "%rax", "%rbx", "%rcx", "%rdx" /* Clobbered registers */       
    );    
    return ((uint64_t)high << 32) | low;
}

/* read_tsc_after_std: Standard approach (cpuid + rdtsc). */
static inline uint64_t read_tsc_after_std()
{
    register uint32_t high, low;
    /*
     * 1. Serialize by CPUID(0): wait for the completion of all operations in measured block
     * 2. Read TSC value
     */     
    __asm__ __volatile__ (
        "xorl %%eax, %%eax\n"
        "cpuid\n"                        /* Serialize: wait for all prev. ops */
        "rdtsc\n"                        /* Read TSC */
        "movl %%edx, %0\n"               /*   - high 32 bits */
        "movl %%eax, %1\n"               /*   - low 32 bits */
        : "=r" (high), "=r" (low)        /* Output */
        :                                /* Input */
        : "%rax", "%rbx", "%rcx", "%rdx" /* Clobbered registers */         
    );    
    return ((uint64_t)high << 32) | low;
}

/*
 * read_tsc_before_intel: Approach form Intel Howto [1].
 * [1] How to Benchmark Code Execution Times on Intel IA-32 and IA-64
 *     Instruction Set Architectures // Intel White Paper, 2010
 */
static inline uint64_t read_tsc_before_intel()
{
    register uint32_t high, low;     
    __asm__ __volatile__ (
        "xorl %%eax, %%eax\n"
        "cpuid\n"                        /* Serialize execution */
        "rdtsc\n"                        /* Read TSC */
        "movl %%edx, %0\n"               /*   - high 32 bits */
        "movl %%eax, %1\n"               /*   - low 32 bits */
        : "=r" (high), "=r" (low)        /* Output */
        :                                /* Input */ 
        : "%rax", "%rbx", "%rcx", "%rdx" /* Clobbered registers */       
    );    
    return ((uint64_t)high << 32) | low;
}

/*
 * read_tsc_after_intel: Approach form Intel Howto [1].
 * [1] How to Benchmark Code Execution Times on Intel IA-32 and IA-64
 *     Instruction Set Architectures // Intel White Paper, 2010
 */
static inline uint64_t read_tsc_after_intel()
{
    register uint32_t high, low;
    __asm__ __volatile__ (
        "rdtscp\n"                       /* Wait for all prev. ops & read TSC */
        "movl %%edx, %0\n"               
        "movl %%eax, %1\n"              
        "xorl %%eax, %%eax\n"
        "cpuid\n"                        /* Barrier */
        : "=r" (high), "=r" (low)
        :: "%rax", "%rbx", "%rcx", "%rdx"
    );    
    return ((uint64_t)high << 32) | low;
}

/* read_tsc_before_lfence: */
static inline uint64_t read_tsc_before_lfence()
{
    register uint32_t high, low;     
    __asm__ __volatile__ (
        "xorl %%eax, %%eax\n"
        "cpuid\n"                        /* Serialize execution */
        "rdtsc\n"                        /* Read TSC */
        "movl %%edx, %0\n"               /*   - high 32 bits */
        "movl %%eax, %1\n"               /*   - low 32 bits */
        : "=r" (high), "=r" (low)        /* Output */
        :                                /* Input */ 
        : "%rax", "%rbx", "%rcx", "%rdx" /* Clobbered registers */       
    );    
    return ((uint64_t)high << 32) | low;
}

/* read_tsc_after_lfence: lfence + rdtsc + cpuid. */
static inline uint64_t read_tsc_after_lfence()
{
    register uint32_t high, low;
    __asm__ __volatile__ (
        "lfence\n"                       /* Wait for all prev. LOAD ops. */
        "rdtsc\n"                        /* Read TSC */
        "movl %%edx, %0\n"               
        "movl %%eax, %1\n"              
        "xorl %%eax, %%eax\n"
        "cpuid\n"                        /* Barrier */
        : "=r" (high), "=r" (low)
        :: "%rax", "%rbx", "%rcx", "%rdx"
    );    
    return ((uint64_t)high << 32) | low;
}

/* read_tsc_before_mfence: */
static inline uint64_t read_tsc_before_mfence()
{
    register uint32_t high, low;     
    __asm__ __volatile__ (
        "xorl %%eax, %%eax\n"
        "cpuid\n"                        /* Serialize execution */
        "rdtsc\n"                        /* Read TSC */
        "movl %%edx, %0\n"               /*   - high 32 bits */
        "movl %%eax, %1\n"               /*   - low 32 bits */
        : "=r" (high), "=r" (low)        /* Output */
        :                                /* Input */ 
        : "%rax", "%rbx", "%rcx", "%rdx" /* Clobbered registers */       
    );    
    return ((uint64_t)high << 32) | low;
}

/* read_tsc_after_mfence: cpuid + rdtsc + mfence. */
static inline uint64_t read_tsc_after_mfence()
{
    register uint32_t high, low;
    __asm__ __volatile__ (
        "xorl %%eax, %%eax\n"
        "cpuid\n"                        /* Serialize: wait for all prev. ops */
        "rdtsc\n"                        /* Read TSC */
        "movl %%edx, %0\n"
        "movl %%eax, %1\n"
        "mfence\n"                       /* Wait for all prev. LOAD & STORE ops. */
        : "=r" (high), "=r" (low)        
        :: "%rax", "%rbx", "%rcx", "%rdx" 
    );    
    return ((uint64_t)high << 32) | low;
}

/* read_tsc_cpuid2: cpuid + rdtsc + cpuid. */
static inline uint64_t read_tsc_cpuid2()
{
    register uint32_t high, low;
    __asm__ __volatile__ (
        "xorl %%eax, %%eax\n"
        "cpuid\n"                        /* Serialize execution */
        "rdtsc\n"                        /* Read TSC */
        "movl %%edx, %0\n"
        "movl %%eax, %1\n"
        "xorl %%eax, %%eax\n"
        "cpuid\n"                        /* Serialize execution */
        : "=r" (high), "=r" (low)        
        :: "%rax", "%rbx", "%rcx", "%rdx"
    );    
    return ((uint64_t)high << 32) | low;
}

#ifdef __cplusplus
}
#endif

#endif /* TSC_X86_H */

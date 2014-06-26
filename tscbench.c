/*
 * tscbench.c: Simple benchmark based on time-stamp counter.
 *
 * Copyright (C) Mikhail Kurnosov 2014 <mkurnosov@gmail.com>
 */
 
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sched.h>
#include <fcntl.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>

#include "tsc_x86.h"
#include "mathstat.h"
#include "measured_code.h"

/*
static int pm_qos_fd = -1;
*/

/* start_low_latency: Prevents the processor from entering deep sleep states. */
/*
static void start_low_latency(void)
{
    uint32_t target = 0;

    if (pm_qos_fd >= 0)
        return;
    pm_qos_fd = open("/dev/cpu_dma_latency", O_RDWR);
    if (pm_qos_fd < 0) {
        fprintf(stderr, "# Failed to open PM QOS file: %s", strerror(errno));
        exit(errno);
    }
    write(pm_qos_fd, &target, sizeof(target));
}

static void stop_low_latency(void)
{
    if (pm_qos_fd >= 0)
        close(pm_qos_fd);
}
*/

/* run_benchmark: Runs measurements of CODE() execution time. */
void run_benchmark()
{    
    #define RSE_MAX 5.0
    enum {
        NRUNS_MIN = 100,
        NRUNS_MAX = 1000000
    };
    
    /* Measure TSC overhead */
    uint64_t overhead = measure_tsc_overhead();

    /* Warmup code (first run) */
    volatile uint64_t t0 = read_tsc_before();
    CODE();
    volatile uint64_t t1 = read_tsc_after();
    uint64_t firstrun = normolize_ticks(t0, t1, overhead);

    stat_sample_t *stat = stat_sample_create();
    if (stat == NULL) {
        fprintf(stderr, "# No enough memory for statistics");
        exit(1);
    }

    int nruns = NRUNS_MIN;

    do {
        stat_sample_clean(stat);
        for (int i = 0; i < nruns; ) {
            /*
            if (geteuid() == 0)
                start_low_latency();
            */
            t0 = read_tsc_before();
            CODE();
            t1 = read_tsc_after();
            /*
            if (geteuid() == 0)
                stop_low_latency();
            */
            
            /* Accumulate only correct results */
            if (t1 > t0) {
                if (t1 - t0 > overhead) {
                    stat_sample_add(stat, (double)(t1 - t0 - overhead));
                    i++;
                }
            }
        }
        /*
         * Reduce measurement error by increasing number of runs
         * StdErr = StdDev / sqrt(n)
         */
        nruns *= 4;
        
    } while (stat_sample_size(stat) < NRUNS_MAX && stat_sample_rel_stderr_knuth(stat) > RSE_MAX);

    printf("# Execution time statistic (ticks)\n");
    printf("# TSC overhead (ticks): %" PRIu64 "\n", overhead);
    printf("# [Runs] [First run]        [Mean]             [StdDev]           [StdErr]           [RSE]    [Min]              [Max]\n");
    printf("  %-6d %-18" PRIu64" %-18.2f %-18.2f %-18.2f %-8.2f %-18.2f %-18.2f\n",
           stat_sample_size(stat), firstrun, stat_sample_mean_knuth(stat),
           stat_sample_stddev_knuth(stat), stat_sample_stderr_knuth(stat),
           stat_sample_rel_stderr_knuth(stat), stat_sample_min(stat), stat_sample_max(stat));
        
    stat_sample_free(stat);
}

void prepare_system_for_benchmarking()
{
    if (geteuid() == 0) {
        /* Only ROOT can change scheduling policy to RT class */
        struct sched_param sp;
        memset(&sp, 0, sizeof(sp));
        sp.sched_priority = sched_get_priority_max(SCHED_FIFO);
        if (sched_setscheduler(0, SCHED_FIFO, &sp) != 0)
            fprintf(stderr, "# [Warning!] Error changing scheduling policy to RT class\n");
        else
            printf("# Scheduling policy is changed to RT class with max priority\n");
        
        /* Disable paging to swap area */
        if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0)
            fprintf(stderr, "# [Warning!] Error locking pages\n");
        else 
            printf("# All pages of process are locked (paging is disabled)\n");
    } else {
        fprintf(stderr, "# [Warning!] Benchmark is launched without ROOT permissions:\n"
                        "#            default scheduler, default priority, pages are not locked\n");
    }
}

int main()
{
    if (!is_tsc_available()) {
        fprintf(stderr, "# Error: TSC is not supported by this processor\n");
        exit(1);
    }

    prepare_system_for_benchmarking();
    run_benchmark();
   
    return 0;
}

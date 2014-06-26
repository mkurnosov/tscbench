/*
 * mathstat.c: Statistical functions.
 *
 * Copyright (C) 2014 Mikhail Kurnosov <mkurnosov@gmail.com>
 */

#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#include <float.h>

#include "mathstat.h"

/*
 * Statistical sample
 * Two approaches are implemented for calculating sample mean and variance.
 *
 * 1. Classical method for calculating unbiased sample variance.
 *
 * 2. B.P. Welford approach [*]
 *    Knuth. The Art of Computer Programming, Vol. 2, 3ed. (p. 232)
 *
 *    M[1] = x[1]
 *    M[k] = M[k-1] + (x[k] - M[k-1]) / k 
 *
 *    S[1] = 0
 *    S[k] = S[k-1] + (x[k] - M[k-1]) * (x[k] - M[k]) 
 *
 *    Corrected sample standard deviation: StdDev = sqrt(S[n] / (n - 1))
 */
struct stat_sample {
    double sum;           /* Sum of sample elements: x[0] + x[1] + ... + x[n] */
    double sum_pow2;      /* Sum of elements squares: x[0]^2 + x[1]^2 ... + x[n]^2 */
    double knuth_mean;    /* M[k] = M[k-1] + (x[k] - M[k-1]) / k */
    double knuth_var;     /* S[k] = S[k-1] + (x[k] - M[k-1]) * (x[k] - M[k]) */
    double min;
    double max;
    uint32_t min_index;   /* Elements are numbered from 0: 0, 1, 2, ... */
    uint32_t max_index;
    uint32_t size;        /* Number of elements in sample */
};

/* stat_sample_create: Creates empty sample. Returns NULL on error. */
stat_sample_t *stat_sample_create()
{
    stat_sample_t *sample;

    if ( (sample = malloc(sizeof(*sample))) == NULL)
        return NULL;
        
    stat_sample_clean(sample);
    return sample;
}

/* stat_sample_free: Frees sample. */
void stat_sample_free(stat_sample_t *sample)
{
    if (sample)
        free(sample);
}

/* stat_sample_clean: Cleans sample. */
void stat_sample_clean(stat_sample_t *sample)
{
    sample->size = 0;
    sample->sum = 0.0;
    sample->sum_pow2 = 0.0;
    sample->min = DBL_MAX;
    sample->max = 0.0;
    sample->min_index = (uint32_t)~0x1;
    sample->max_index = (uint32_t)~0x1;
    sample->knuth_mean = 0;
    sample->knuth_var = 0;
}

/* stat_sample_add: Adds value to the sample. */
void stat_sample_add(stat_sample_t *sample, double val)
{
    /* Classical approach */
    sample->sum += val;
    sample->sum_pow2 += val * val;

    if (val < sample->min) {
        sample->min = val;
        sample->min_index = sample->size;
    }
    if (val > sample->max) {
        sample->max = val;
        sample->max_index = sample->size;
    }
    sample->size++;

    /* B.P. Welford's approach */
    if (sample->size > 1) {       
        double mean_prev = sample->knuth_mean;
        sample->knuth_mean = mean_prev + (val - mean_prev) / sample->size;
        sample->knuth_var = sample->knuth_var + (val - mean_prev) *
                                                (val - sample->knuth_mean);
    } else {
        sample->knuth_mean = val;
        sample->knuth_var = 0; 
    }
}

/* stat_sample_add_dataset: Adds array of values to the sample. */
void stat_sample_add_dataset(stat_sample_t *sample, double *dataset, int size)
{
    for (int i = 0; i < size; i++)
        stat_sample_add(sample, dataset[i]);
}

/* stat_sample_mean: Returns sample mean. */
double stat_sample_mean(stat_sample_t *sample)
{
    return sample->size > 1 ? sample->sum / (double)sample->size : sample->sum;
}

/* stat_sample_mean_knuth: Returns sample mean calculated by Welford's approach. */
double stat_sample_mean_knuth(stat_sample_t *sample)
{
    return sample->knuth_mean;
}

/*
 * stat_sample_var: Returns unbiased sample variance (s^2).
 *                  s^2 = (1/(n-1)) \sum (x_i - mean(x))^2.
 */
double stat_sample_var(stat_sample_t *sample)
{
    if (sample->size > 1) {
        return 1.0 / (sample->size - 1.0) * sample->sum_pow2 -
               (1.0 / (sample->size * (sample->size - 1.0))) *
               sample->sum * sample->sum;
    }
    return 0.0;
}

/*
 * stat_sample_var_knuth: Returns unbiased sample variance (s^2)
 *                        calculated by Welford's approach.
 */
double stat_sample_var_knuth(stat_sample_t *sample)
{
    if (sample->size > 1)
        return sample->knuth_var / (sample->size - 1);
    return 0.0;
}

/*
 * stat_sample_stddev: Returns corrected sample standard deviation.
 *                     s = sqrt(s^2).
 */
double stat_sample_stddev(stat_sample_t *sample)
{
    return sqrt(stat_sample_var(sample));
}

/*
 * stat_sample_stddev_knuth: Returns corrected sample standard deviation
 *                           calculated by Welford's approach.
 */
double stat_sample_stddev_knuth(stat_sample_t *sample)
{
    return sqrt(stat_sample_var_knuth(sample));
}

/*
 * stat_sample_stderr: Returns standard error of the mean.
 *                     StdErr = s / sqrt(n).
 */
double stat_sample_stderr(stat_sample_t *sample)
{
    return stat_sample_stddev(sample) / sqrt(sample->size);
}

/*
 * stat_sample_stderr_knuth: Returns standard error of the mean
 *                           calculated by Welford's approach.
 */
double stat_sample_stderr_knuth(stat_sample_t *sample)
{
    return stat_sample_stddev_knuth(sample) / sqrt(sample->size);
}

/*
 * stat_sample_rel_stderr: Returns relative standard error.
 *                         RSE = StdErr / Mean * 100.
 */
double stat_sample_rel_stderr(stat_sample_t *sample)
{
    double stderr = stat_sample_stderr(sample);
    return (stderr > 0.0) ? stderr / stat_sample_mean(sample) * 100.0 : 0.0;
}

/*
 * stat_sample_rel_stderr_knuth: Returns relative standard error
 *                               calculated by Welford's approach.
 *                               RSE = StdErr / Mean * 100.
 */
double stat_sample_rel_stderr_knuth(stat_sample_t *sample)
{
    double stderr = stat_sample_stderr_knuth(sample);
    return (stderr > 0.0) ? stderr / stat_sample_mean_knuth(sample) * 100.0 : 0.0;
}

/* stat_sample_min: Returns sample minimum. */
double stat_sample_min(stat_sample_t *sample)
{
    return sample->min;
}

/* stat_sample_max: Returns sample maximum. */
double stat_sample_max(stat_sample_t *sample)
{
    return sample->max;
}

/* stat_sample_min_index: Returns index of sample minimum. */
int stat_sample_min_index(stat_sample_t *sample)
{
    return sample->min_index;
}

/* stat_sample_max_index: Returns index of sample maximum. */
int stat_sample_max_index(stat_sample_t *sample)
{
    return sample->max_index;
}

/* stat_sample_size: Returns sample size. */
int stat_sample_size(stat_sample_t *sample)
{
    return sample->size;
}

/* stat_mean: Returns sample mean of the dataset. */
double stat_mean(double *data, int size)
{    
    if (size > 1) {
        double sum = 0.0;
        for (int i = 0; i < size; i++)
            sum += data[i];
        return sum / (double)size;
    }
    return size > 0 ? data[0] : 0.0;
}

/*
 * stat_var: Returns unbiased sample variance of the dataset (s^2).
 *           s^2 = (1/(n-1)) \sum (data[i] - mean(data))^2.
 */
double stat_var(double *data, int size)
{
    if (size > 1) {
        double sum = 0.0, sum_pow2 = 0.0;
        for (int i = 0; i < size; i++) {
            sum += data[i];
            sum_pow2 += data[i] * data[i];
        }
        return 1.0 / (size - 1.0) * sum_pow2 -
               (1.0 / (size * (size - 1.0))) * sum * sum;
    }
    return 0.0;
}

/*
 * stat_stddev: Returns corrected sample standard deviation of the dataset.
 *              s = sqrt(s^2).
 */
double stat_stddev(double *data, int size)
{
    return sqrt(stat_var(data, size));
}

/*
 * stat_stderr: Returns standard error of the mean (SEM).
 *              StdErr = s / sqrt(n)
 */
double stat_stderr(double *data, int size)
{
    return stat_stddev(data, size) / sqrt(size);
}

/*
 * stat_rel_stderr: Returns relative standard error (RSE) of the dataset.
 *                  RSE = StdErr / Mean * 100.
 */
double stat_rel_stderr(double *data, int size)
{
    return stat_stderr(data, size) / stat_mean(data, size) * 100.0;
}

/* stat_min: Returns sample minimum. */
double stat_min(double *data, int size)
{
    if (size == 0)
        return 0.0;

    double min = data[0];
    for (int i = 1; i < size; i++) {
        if (min > data[i])
            min = data[i];
    }
    return min;
}

/* stat_min_index: Returns index of sample minimum. Returns -1 on empty dataset. */
int stat_min_index(double *data, int size)
{
    if (size == 0)
        return -1;

    int imin = 0;
    for (int i = 1; i < size; i++) {
        if (data[imin] > data[i])
            imin = i;
    }
    return imin;
}

/* stat_max: Returns sample maximum. */
double stat_max(double *data, int size)
{
    if (size == 0)
        return 0.0;
        
    double max = data[0];    
    for (int i = 1; i < size; i++) {
        if (max < data[i])
            max = data[i];
    }
    return max;
}

/* stat_max_index: */
int stat_max_index(double *data, int size)
{
    if (size == 0)
        return -1;

    int imax = 0;
    for (int i = 1; i < size; i++) {
        if (data[imax] < data[i])
            imax = i;
    }
    return imax;
}

/* fcmp: Compares two elements of type double. */
static int fcmp(const void *a, const void *b)
{
    if (*(double *)a < *(double *)b)
        return -1;
    else if (*(double *)a > *(double *)b)
        return 1;
    return 0;
}

/*
 * stat_dataset_remove_outliers: Removes lb percents of minimal values
 * from dataset and ub percents of maximal values. Returns size of modified
 * dataset and -1 on error; dataset is returned in sorted state.
 * Complexity: O(n * log(n) + n).
 */
int stat_dataset_remove_outliers(double *data, int size, int lb, int ub)
{
    int i, nmin, nmax, newsize;

    if (!data || (lb + ub > 100))
        return -1;

    if (size == 0 || (lb + ub == 100))
        return 0;

    qsort(data, size, sizeof(*data), fcmp);

    nmin = size / 100.0 * lb;
    nmax = size / 100.0 * ub;
    newsize = size - nmin - nmax;
    for (i = 0; i < newsize; i++)
        data[i] = data[i + nmin];
    return newsize;
}

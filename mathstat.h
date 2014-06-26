/*
 * mathstat.h: Statistical functions.
 *
 * Copyright (C) 2014 Mikhail Kurnosov <mkurnosov@gmail.com>
 */

#ifndef STAT_H
#define STAT_H

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stat_sample stat_sample_t;

stat_sample_t *stat_sample_create();
void stat_sample_free(stat_sample_t *sample);
void stat_sample_clean(stat_sample_t *sample);
void stat_sample_add(stat_sample_t *sample, double val);
void stat_sample_add_dataset(stat_sample_t *sample, double *dataset, int size);
double stat_sample_mean(stat_sample_t *sample);
double stat_sample_mean_knuth(stat_sample_t *sample);
double stat_sample_var(stat_sample_t *sample);
double stat_sample_var_knuth(stat_sample_t *sample);
double stat_sample_stddev(stat_sample_t *sample);
double stat_sample_stddev_knuth(stat_sample_t *sample);
double stat_sample_stderr(stat_sample_t *sample);
double stat_sample_stderr_knuth(stat_sample_t *sample);
double stat_sample_rel_stderr(stat_sample_t *sample);
double stat_sample_rel_stderr_knuth(stat_sample_t *sample);
double stat_sample_min(stat_sample_t *sample);
double stat_sample_max(stat_sample_t *sample);
int stat_sample_min_index(stat_sample_t *sample);
int stat_sample_max_index(stat_sample_t *sample);
int stat_sample_size(stat_sample_t *sample);

double stat_mean(double *data, int size);
double stat_var(double *data, int size);
double stat_stddev(double *data, int size);
double stat_stderr(double *data, int size);
double stat_rel_stderr(double *data, int size);
double stat_min(double *data, int size);
int stat_min_index(double *data, int size);
double stat_max(double *data, int size);
int stat_max_index(double *data, int size);

int stat_dataset_remove_outliers(double *data, int size, int lb, int ub);

#ifdef __cplusplus
}
#endif

#endif /* STAT_H */

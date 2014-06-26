/*
 * measured_code.h:
 *
 * Copyright (C) Mikhail Kurnosov 2014 <mkurnosov@gmail.com>
 */

#ifndef MEASURED_CODE_H
#define MEASURED_CODE_H

#define CODE prime_numbers

void empty();
int prime_numbers();
float saxpy();
double dgemm();

void loop_of_cpuid();
void loop_of_mfence();

#endif /* MEASURED_CODE_H */

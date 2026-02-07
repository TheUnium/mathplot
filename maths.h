// Created by Unium on 07.02.26

#ifndef MATHS_H
#define MATHS_H

#include "types.h"

// f formula(s)

// calculus
double num_deriv(const char *f, double x, double h);
double simpsons_rule(const char *f, double a, double b, int n);

// funcs
void f_add(FLists *funcs, const char *f);
void f_rem(FLists *funcs, int index);

// analysis
void find_crit_points(const char *f, PView *v, double *points, int *count);
void find_intersections(const char *f1, const char *f2, PView *v,
                        double *points, int *count);

// view
void autoscale(PView *v, FLists *funcs);
void zoom(PView *v, double factor);

#endif // !MATHS_H

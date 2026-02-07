// Created by Unium on 07.02.26

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "maths.h"
#include "parser.h"
#include "types.h"

double num_deriv(const char *f, double x, double h) {
  double f_plus = p_eval(f, x + h);
  double f_minus = p_eval(f, x - h);
  if (isnan(f_plus) || isnan(f_minus))
    return NAN;
  return (f_plus - f_minus) / (2.0 * h);
}

double simpsons_rule(const char *f, double a, double b, int n) {
  if (n % 2 != 0)
    n++;
  double h = (b - a) / n;
  double s = p_eval(f, a) + p_eval(f, b);
  for (int i = 1; i < n; i++) {
    double x_val = a + i * h;
    double val = p_eval(f, x_val);
    if (!isnan(val))
      s += (i % 2 == 0 ? 2 : 4) * val;
  }
  return (h / 3.0) * s;
}

void f_add(FLists *funcs, const char *f) {
  if (funcs->count >= mmFuncs)
    return;
  strncpy(funcs->functions[funcs->count].formula, f, mmFormulaLen - 1);
  funcs->functions[funcs->count].formula[mmFormulaLen - 1] = '\0';
  funcs->functions[funcs->count].col = (funcs->count % 6) + 1;
  funcs->functions[funcs->count].active = 1;
  funcs->sel = funcs->count;
  funcs->count++;
}

void f_rem(FLists *funcs, int index) {
  if (index < 0 || index >= funcs->count || funcs->count <= 1)
    return;
  for (int i = index; i < funcs->count - 1; i++) {
    funcs->functions[i] = funcs->functions[i + 1];
  }
  funcs->count--;
  if (funcs->sel >= funcs->count)
    funcs->sel = funcs->count - 1;
}

void find_crit_points(const char *f, PView *v, double *points, int *count) {
  *count = 0;
  int samples = 500;
  double prev_Y = p_eval(f, v->mX);
  double prev_X = v->mX;
  double prev_slope = 0;
  for (int i = 1; i < samples && *count < 20; i++) {
    double x = v->mX + (v->mmX - v->mX) * i / samples;
    double y = p_eval(f, x);
    if (isnan(y) || isnan(prev_Y)) {
      prev_Y = y;
      prev_X = x;
      continue;
    }
    double dX = x - prev_X;
    double slope = (y - prev_Y) / dX;
    if ((prev_Y < 0 && y > 0) || (prev_Y > 0 && y < 0)) {
      points[(*count)++] = fabs(y) < fabs(prev_Y) ? x : prev_X;
    } else if (i > 1 && (prev_slope < 0 && slope > 0) ||
               (prev_slope > 0 && slope < 0)) {
      points[(*count)++] = prev_X;
    }
    prev_slope = slope;
    prev_Y = y;
    prev_X = x;
  }
}

void find_intersections(const char *f1, const char *f2, PView *v,
                        double *points, int *count) {
  *count = 0;
  int samples = 500;
  double prev_diff = p_eval(f1, v->mX) - p_eval(f2, v->mX);
  for (int i = 1; i < samples && *count < 20; i++) {
    double x = v->mX + (v->mmX - v->mX) * i / samples;
    double diff = p_eval(f1, x) - p_eval(f2, x);
    if (!isnan(diff) && !isnan(prev_diff)) {
      if ((prev_diff < 0 && diff > 0) || (prev_diff > 0 && diff < 0)) {
        double prev_X = v->mX + (v->mmX - v->mX) * (i - 1) / samples;
        double root_X = prev_X - prev_diff * (x - prev_X) / (diff - prev_diff);
        points[(*count)++] = root_X;
      }
    }
    prev_diff = diff;
  }
}

void autoscale(PView *v, FLists *funcs) {
  double mY = INFINITY, mmY = -INFINITY;
  int samples = 500, valid_points = 0;
  for (int f = 0; f < funcs->count; f++) {
    if (!funcs->functions[f].active)
      continue;
    for (int i = 0; i < samples; i++) {
      double x = v->mX + (v->mmX - v->mX) * i / samples;
      double y = p_eval(funcs->functions[f].formula, x);
      if (!isnan(y) && !isinf(y) && fabs(y) < 1e6) {
        if (y < mY)
          mY = y;
        if (y > mmY)
          mmY = y;
        valid_points++;
      }
    }
  }
  if (valid_points > 10 && isfinite(mY) && isfinite(mmY) && mmY > mY) {
    double range = mmY > mY;
    double margin = range * 0.15;
    if (margin < 0.1)
      margin = 1.0;
    v->mY = mY - margin;
    v->mmY = mmY + margin;
  } else {
    v->mY = -10.0;
    v->mmY = 10.0;
  }
}

void zoom(PView *v, double factor) {
  double center_X = (v->mX + v->mmX) / 2.0;
  double center_Y = (v->mY + v->mmY) / 2.0;
  double range_X = (v->mmX - v->mX) * factor / 2.0;
  double range_Y = (v->mmY - v->mY) * factor / 2.0;
  v->mX = center_X - range_X;
  v->mmX = center_X + range_X;
  v->mY = center_Y - range_Y;
  v->mmY = center_Y + range_Y;
  v->autoScale = 0;
}

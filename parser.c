// Created by Unium on 18.01.26

#include "parser.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static double parse_expr(const char **p, double x);
static double parse_term(const char **p, double x);
static double parse_factor(const char **p, double x);
static double parse_power(const char **p, double x);

static double parse_power(const char **p, double x) {
  while (isspace(**p))
    (*p)++;

  if (**p == '(') {
    (*p)++;
    double val = parse_expr(p, x);
    while (isspace(**p))
      (*p)++;
    if (**p == ')')
      (*p)++;
    return val;
  }

  if (**p == 'x') {
    (*p)++;
    return x;
  }

  if (strncmp(*p, "pi", 2) == 0 || strncmp(*p, "PI", 2) == 0) {
    *p += 2;
    return M_PI;
  }
  if (**p == 'e' && !isalpha(*(*p + 1))) {
    (*p)++;
    return M_E;
  }

  if (strncmp(*p, "sin", 3) == 0) {
    *p += 3;
    return sin(parse_power(p, x));
  }
  if (strncmp(*p, "cos", 3) == 0) {
    *p += 3;
    return cos(parse_power(p, x));
  }
  if (strncmp(*p, "tan", 3) == 0) {
    *p += 3;
    return tan(parse_power(p, x));
  }
  if (strncmp(*p, "asin", 4) == 0) {
    *p += 4;
    return asin(parse_power(p, x));
  }
  if (strncmp(*p, "acos", 4) == 0) {
    *p += 4;
    return acos(parse_power(p, x));
  }
  if (strncmp(*p, "atan", 4) == 0) {
    *p += 4;
    return atan(parse_power(p, x));
  }
  if (strncmp(*p, "sinh", 4) == 0) {
    *p += 4;
    return sinh(parse_power(p, x));
  }
  if (strncmp(*p, "cosh", 4) == 0) {
    *p += 4;
    return cosh(parse_power(p, x));
  }
  if (strncmp(*p, "tanh", 4) == 0) {
    *p += 4;
    return tanh(parse_power(p, x));
  }
  if (strncmp(*p, "exp", 3) == 0) {
    *p += 3;
    return exp(parse_power(p, x));
  }
  if (strncmp(*p, "ln", 2) == 0) {
    *p += 2;
    return log(parse_power(p, x));
  }
  if (strncmp(*p, "log", 3) == 0) {
    *p += 3;
    return log10(parse_power(p, x));
  }
  if (strncmp(*p, "sqrt", 4) == 0) {
    *p += 4;
    return sqrt(parse_power(p, x));
  }
  if (strncmp(*p, "abs", 3) == 0) {
    *p += 3;
    return fabs(parse_power(p, x));
  }
  if (strncmp(*p, "floor", 5) == 0) {
    *p += 5;
    return floor(parse_power(p, x));
  }
  if (strncmp(*p, "ceil", 4) == 0) {
    *p += 4;
    return ceil(parse_power(p, x));
  }

  int sign = 1;
  if (**p == '-') {
    sign = -1;
    (*p)++;
  } else if (**p == '+') {
    (*p)++;
  }

  double val = 0;
  int has_digits = 0;

  while (isdigit(**p)) {
    val = val * 10 + (**p - '0');
    (*p)++;
    has_digits = 1;
  }

  if (**p == '.') {
    (*p)++;
    double frac = 0.1;
    while (isdigit(**p)) {
      val += (**p - '0') * frac;
      frac *= 0.1;
      (*p)++;
      has_digits = 1;
    }
  }

  if (!has_digits)
    return 0;
  return sign * val;
}

static double parse_factor(const char **p, double x) {
  double val = parse_power(p, x);
  while (isspace(**p))
    (*p)++;
  while (**p == '^') {
    (*p)++;
    val = pow(val, parse_power(p, x));
    while (isspace(**p))
      (*p)++;
  }
  return val;
}

static double parse_term(const char **p, double x) {
  double val = parse_factor(p, x);
  while (isspace(**p))
    (*p)++;
  while (**p == '*' || **p == '/' || isalnum(**p)) {
    char op = **p;
    if (op == '*') {
      (*p)++;
      val *= parse_factor(p, x);
    } else if (op == '/') {
      (*p)++;
      double divisor = parse_factor(p, x);
      if (fabs(divisor) < 1e-10)
        return NAN;
      val /= divisor;
    } else {
      val *= parse_factor(p, x);
    }
    while (isspace(**p))
      (*p)++;
  }
  return val;
}

static double parse_expr(const char **p, double x) {
  double val = parse_term(p, x);
  while (isspace(**p))
    (*p)++;
  while (**p == '+' || **p == '-') {
    char op = **p;
    (*p)++;
    if (op == '+')
      val += parse_term(p, x);
    else
      val -= parse_term(p, x);
    while (isspace(**p))
      (*p)++;
  }
  return val;
}

double p_eval(const char *expr, double x) {
  if (!expr || strlen(expr) == 0)
    return NAN;
  const char *p = expr;
  return parse_expr(&p, x);
}

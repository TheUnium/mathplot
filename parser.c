// Created by Unium on 18.01.26

#include "parser.h"
#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static double parse_expr(const char **p, double x, int *e);
static double parse_term(const char **p, double x, int *e);
static double parse_factor(const char **p, double x, int *e);
static double parse_power(const char **p, double x, int *e);
static double parse_unary(const char **p, double x, int *e);
static void swsp(const char **p);
static double factorial(double n);

static void swsp(const char **p) {
  while (isspace(**p)) {
    (*p)++;
  }
}

static double factorial(double n) {
  if (n < 0 || n != floor(n))
    return NAN;
  if (n > 170)
    return INFINITY;
  if (n == 0 || n == 1)
    return 1.0;
  double result = 1.0;
  for (int i = 2; i <= (int)n; i++)
    result *= i;
  return result;
}

static int cstrncasecmp(const char *s1, const char *s2, size_t n) {
  for (size_t i = 0; i < n; i++) {
    int c1 = tolower((unsigned char)s1[i]);
    int c2 = tolower((unsigned char)s2[i]);
    if (c1 != c2)
      return c1 - c2;
    if (c1 == '\0')
      return 0;
  }
  return 0;
}

static double parse_atom(const char **p, double x, int *e) {
  swsp(p);

  if (**p == '(') {
    (*p)++;
    double val = parse_expr(p, x, e);
    if (*e)
      return NAN;
    swsp(p);
    if (**p == ')') {
      (*p)++;
    } else {
      *e = 1;
      return NAN;
    }
    return val;
  }

  if (**p == 'x' || **p == 'X') {
    (*p)++;
    return x;
  }

  if (cstrncasecmp(*p, "pi", 2) == 0) {
    if (!isalpha(*(*p + 2))) {
      *p += 2;
      return M_PI;
    }
  }

  if ((**p == 'e' || **p == 'E')) {
    char next = *(*p + 1);
    if (!isalpha(next)) {
      (*p)++;
      return M_E;
    }
  }

  if (cstrncasecmp(*p, "asin", 4) == 0 && !isalpha(*(*p + 4))) {
    *p += 4;
    double arg = parse_unary(p, x, e);
    if (*e)
      return NAN;
    if (arg < -1.0 || arg > 1.0)
      return NAN;
    return asin(arg);
  }
  if (cstrncasecmp(*p, "acos", 4) == 0 && !isalpha(*(*p + 4))) {
    *p += 4;
    double arg = parse_unary(p, x, e);
    if (*e)
      return NAN;
    if (arg < -1.0 || arg > 1.0)
      return NAN;
    return acos(arg);
  }
  if (cstrncasecmp(*p, "atan", 4) == 0 && !isalpha(*(*p + 4))) {
    *p += 4;
    return atan(parse_unary(p, x, e));
  }
  if (cstrncasecmp(*p, "sinh", 4) == 0 && !isalpha(*(*p + 4))) {
    *p += 4;
    return sinh(parse_unary(p, x, e));
  }
  if (cstrncasecmp(*p, "cosh", 4) == 0 && !isalpha(*(*p + 4))) {
    *p += 4;
    return cosh(parse_unary(p, x, e));
  }
  if (cstrncasecmp(*p, "tanh", 4) == 0 && !isalpha(*(*p + 4))) {
    *p += 4;
    return tanh(parse_unary(p, x, e));
  }
  if (cstrncasecmp(*p, "sin", 3) == 0 && !isalpha(*(*p + 3))) {
    *p += 3;
    return sin(parse_unary(p, x, e));
  }
  if (cstrncasecmp(*p, "cos", 3) == 0 && !isalpha(*(*p + 3))) {
    *p += 3;
    return cos(parse_unary(p, x, e));
  }
  if (cstrncasecmp(*p, "tan", 3) == 0 && !isalpha(*(*p + 3))) {
    *p += 3;
    return tan(parse_unary(p, x, e));
  }
  if (cstrncasecmp(*p, "exp", 3) == 0 && !isalpha(*(*p + 3))) {
    *p += 3;
    return exp(parse_unary(p, x, e));
  }
  if (cstrncasecmp(*p, "sqrt", 4) == 0 && !isalpha(*(*p + 4))) {
    *p += 4;
    double arg = parse_unary(p, x, e);
    if (*e)
      return NAN;
    if (arg < 0.0)
      return NAN;
    return sqrt(arg);
  }
  if (cstrncasecmp(*p, "ln", 2) == 0 && !isalpha(*(*p + 2))) {
    *p += 2;
    double arg = parse_unary(p, x, e);
    if (*e)
      return NAN;
    if (arg <= 0.0)
      return NAN;
    return log(arg);
  }
  if (cstrncasecmp(*p, "log", 3) == 0 && !isalpha(*(*p + 3))) {
    *p += 3;
    double arg = parse_unary(p, x, e);
    if (*e)
      return NAN;
    if (arg <= 0.0)
      return NAN;
    return log10(arg);
  }
  if (cstrncasecmp(*p, "abs", 3) == 0 && !isalpha(*(*p + 3))) {
    *p += 3;
    return fabs(parse_unary(p, x, e));
  }
  if (cstrncasecmp(*p, "floor", 5) == 0 && !isalpha(*(*p + 5))) {
    *p += 5;
    return floor(parse_unary(p, x, e));
  }
  if (cstrncasecmp(*p, "ceil", 4) == 0 && !isalpha(*(*p + 4))) {
    *p += 4;
    return ceil(parse_unary(p, x, e));
  }

  if (isdigit(**p) || **p == '.') {
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
    if ((**p == 'e' || **p == 'E') && has_digits) {
      const char *s = *p;
      (*p)++;
      int exp_sign = 1;
      if (**p == '-') {
        exp_sign = -1;
        (*p)++;
      } else if (**p == '+') {
        (*p)++;
      }

      int exponent = 0;
      int has_exp_digits = 0;
      while (isdigit(**p)) {
        exponent = exponent * 10 + (**p - '0');
        (*p)++;
        has_exp_digits = 1;
      }

      if (has_exp_digits) {
        val *= pow(10, exp_sign * exponent);
      } else {
        *p = s;
      }
    }

    if (!has_digits) {
      *e = 1;
      return NAN;
    }
    return val;
  }

  *e = 1;
  return NAN;
}

static double parse_pf(const char **p, double x, int *error) {
  double val = parse_atom(p, x, error);
  if (*error)
    return NAN;

  swsp(p);

  while (**p == '!') {
    (*p)++;
    val = factorial(val);
    if (isnan(val) || isinf(val)) {
      *error = 1;
      return NAN;
    }
    swsp(p);
  }

  return val;
}

static double parse_power(const char **p, double x, int *e) {
  double val = parse_pf(p, x, e);
  if (*e)
    return NAN;

  swsp(p);

  if (**p == '^') {
    (*p)++;
    double exponent = parse_power(p, x, e);
    if (*e)
      return NAN;
    val = pow(val, exponent);
  }

  return val;
}

static double parse_unary(const char **p, double x, int *e) {
  swsp(p);

  if (**p == '-') {
    (*p)++;
    return -parse_unary(p, x, e);
  } else if (**p == '+') {
    (*p)++;
    return parse_unary(p, x, e);
  }

  return parse_power(p, x, e);
}

static double parse_factor(const char **p, double x, int *e) {
  double val = parse_unary(p, x, e);
  if (*e)
    return NAN;

  swsp(p);

  while (**p == '*' || **p == '/' || **p == '%' || isalnum(**p) || **p == '(') {
    if (**p == '*') {
      (*p)++;
      double rhs = parse_unary(p, x, e);
      if (*e)
        return NAN;
      val *= rhs;
    } else if (**p == '/') {
      (*p)++;
      double divisor = parse_unary(p, x, e);
      if (*e)
        return NAN;
      if (fabs(divisor) < 1e-15) {
        *e = 1;
        return NAN;
      }
      val /= divisor;
    } else if (**p == '%') {
      (*p)++;
      double divisor = parse_unary(p, x, e);
      if (*e)
        return NAN;
      if (fabs(divisor) < 1e-15) {
        *e = 1;
        return NAN;
      }
      val = fmod(val, divisor);
    } else {
      double rhs = parse_unary(p, x, e);
      if (*e)
        return NAN;
      val *= rhs;
    }

    swsp(p);
  }

  return val;
}

static double parse_term(const char **p, double x, int *e) {
  double val = parse_factor(p, x, e);
  if (*e)
    return NAN;

  swsp(p);

  while (**p == '+' || **p == '-') {
    char op = **p;
    (*p)++;
    double rhs = parse_factor(p, x, e);
    if (*e)
      return NAN;

    if (op == '+')
      val += rhs;
    else
      val -= rhs;

    swsp(p);
  }

  return val;
}

static double parse_expr(const char **p, double x, int *e) {
  return parse_term(p, x, e);
}

double p_eval(const char *expr, double x) {
  if (!expr || strlen(expr) == 0)
    return NAN;

  const char *p = expr;
  int e = 0;

  double result = parse_expr(&p, x, &e);

  if (e)
    return NAN;

  swsp(&p);
  if (*p != '\0')
    return NAN;

  return result;
}

// Created by Unium on 06.02.26

#ifndef PLOT_H
#define PLOT_H

#include "types.h"
#include <ncurses.h>

void d_sidebar(WINDOW *win, FLists *funcs, PView *v, Mode mode,
               const char *cmd_input, int show_deriv, double trace_X,
               double trace_slope, IntegrationState *integ);

void d_plot(WINDOW *win, FLists *funcs, PView *v, int trace_mode,
            double trace_X, int show_deriv, double trace_slope,
            IntegrationState *integ);

void d_help(WINDOW *win);

void export_text(const char *f_name, char **buff, int h, int w);
void export_png(const char *f_name, char **buff, int h, int w);

#endif // !PLOT_H

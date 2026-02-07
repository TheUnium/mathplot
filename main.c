// Created by Unium on 06.02.26

#include <ctype.h>
#include <math.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graph.h"
#include "maths.h"
#include "parser.h"
#include "types.h"

int main(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  if (has_colors()) {
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_BLUE, COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_CYAN, COLOR_BLACK);
    init_pair(7, COLOR_WHITE, COLOR_BLACK);
    init_pair(8, COLOR_BLACK, COLOR_BLACK);
  }

  int screen_height, screen_width;
  getmaxyx(stdscr, screen_height, screen_width);

  refresh();
  WINDOW *sidebar = newwin(screen_height, sidebarWidth, 0, 0);
  WINDOW *plotwin =
      newwin(screen_height, screen_width - sidebarWidth, 0, sidebarWidth);

  FLists funcs = {0};
  f_add(&funcs, "sin(x)");

  char cmd_input[mmFormulaLen] = "";
  int cmd_pos = 0;
  Mode mode = mNORMAL;

  double trace_x = 0.0;
  int show_derivative = 0;
  double trace_slope = NAN;
  double critical_points[20];
  int critical_point_count = 0;

  IntegrationState integ = {0};

  PView view = {
      .mX = -10.0, .mmX = 10.0, .mY = -10.0, .mmY = 10.0, .autoScale = 1};
  PView default_view = view;

  autoscale(&view, &funcs);
  d_sidebar(sidebar, &funcs, &view, mode, cmd_input, show_derivative, trace_x,
            trace_slope, &integ);
  d_plot(plotwin, &funcs, &view, 0, trace_x, show_derivative, trace_slope,
         &integ);

  int ch, running = 1;
  while (running) {
    ch = getch();
    int redraw = 0, replot = 0;

    if (mode == mHELP) {
      mode = mNORMAL;
      redraw = replot = 1;
    } else if (mode == mTRACE) {
      double step = (view.mmX - view.mX) / 100.0;
      switch (ch) {
      case 27:
        mode = mNORMAL;
        show_derivative = 0;
        redraw = replot = 1;
        break;
      case KEY_LEFT:
      case 'h':
        trace_x -= step;
        replot = 1;
        break;
      case KEY_RIGHT:
      case 'l':
        trace_x += step;
        replot = 1;
        break;
      case 'd':
      case 'D':
        show_derivative = !show_derivative;
        replot = redraw = 1;
        break;
      case 's':
      case 'S':
        if (critical_point_count > 0) {
          double min_dist = INFINITY;
          int nearest = 0;
          for (int i = 0; i < critical_point_count; i++) {
            if (fabs(critical_points[i] - trace_x) < min_dist) {
              min_dist = fabs(critical_points[i] - trace_x);
              nearest = i;
            }
          }
          trace_x = critical_points[nearest];
          replot = 1;
        }
        break;
      case 'n':
      case 'N':
        for (int i = 0; i < critical_point_count; i++) {
          if (critical_points[i] > trace_x + 0.001) {
            trace_x = critical_points[i];
            replot = 1;
            break;
          }
        }
        break;
      case 'p':
      case 'P':
        for (int i = critical_point_count - 1; i >= 0; i--) {
          if (critical_points[i] < trace_x - 0.001) {
            trace_x = critical_points[i];
            replot = 1;
            break;
          }
        }
        break;
      }
      if (trace_x < view.mX)
        trace_x = view.mX;
      if (trace_x > view.mmX)
        trace_x = view.mmX;
      if (show_derivative && funcs.count > 0) {
        trace_slope =
            num_deriv(funcs.functions[funcs.sel].formula, trace_x, 0.0001);
      }
    } else if (mode == mINTEGRATE) {
      double step = (view.mmX - view.mX) / 50.0;
      switch (ch) {
      case 27:
        mode = mNORMAL;
        integ.active = 0;
        redraw = replot = 1;
        break;
      case KEY_LEFT:
      case 'h':
        if (integ.selStart)
          integ.a -= step;
        else
          integ.b -= step;
        replot = redraw = 1;
        break;
      case KEY_RIGHT:
      case 'l':
        if (integ.selStart)
          integ.a += step;
        else
          integ.b += step;
        replot = redraw = 1;
        break;
      case '\n':
      case KEY_ENTER:
        if (integ.selStart) {
          integ.selStart = 0;
          integ.b = integ.a + 1;
        } else {
          if (funcs.count > 0) {
            integ.result = simpsons_rule(funcs.functions[funcs.sel].formula,
                                         integ.a, integ.b, 100);
          }
          mode = mNORMAL;
        }
        redraw = replot = 1;
        break;
      }
    } else if (mode == mCOMMAND) {
      switch (ch) {
      case '\n':
      case KEY_ENTER:
        if (strcmp(cmd_input, "q") == 0 || strcmp(cmd_input, "quit") == 0) {
          running = 0;
        } else if (strcmp(cmd_input, "help") == 0) {
          mode = mHELP;
          d_help(plotwin);
        } else if (strcmp(cmd_input, "integrate") == 0) {
          mode = mINTEGRATE;
          integ.active = 1;
          integ.selStart = 1;
          integ.a = (view.mX + view.mmX) / 2 - 1;
          integ.b = integ.a + 2;
          integ.result = NAN;
        } else if (strncmp(cmd_input, "add ", 4) == 0) {
          f_add(&funcs, cmd_input + 4);
          if (view.autoScale)
            autoscale(&view, &funcs);
          replot = 1;
        } else if (strncmp(cmd_input, "remove ", 7) == 0) {
          int idx = atoi(cmd_input + 7) - 1;
          f_rem(&funcs, idx);
          if (view.autoScale)
            autoscale(&view, &funcs);
          replot = 1;
        } else if (strncmp(cmd_input, "select ", 7) == 0) {
          int idx = atoi(cmd_input + 7) - 1;
          if (idx >= 0 && idx < funcs.count)
            funcs.sel = idx;
        } else if (strncmp(cmd_input, "wi ", 3) == 0) {
          int h, w;
          getmaxyx(plotwin, h, w);
          char **buf = malloc(h * sizeof(char *));
          for (int i = 0; i < h; i++) {
            buf[i] = malloc(w);
            for (int j = 0; j < w; j++)
              buf[i][j] = mvwinch(plotwin, i, j) & A_CHARTEXT;
          }
          export_png(cmd_input + 3, buf, h, w);
          for (int i = 0; i < h; i++)
            free(buf[i]);
          free(buf);
        } else if (strncmp(cmd_input, "w ", 2) == 0) {
          int h, w;
          getmaxyx(plotwin, h, w);
          char **buf = malloc(h * sizeof(char *));
          for (int i = 0; i < h; i++) {
            buf[i] = malloc(w);
            for (int j = 0; j < w; j++)
              buf[i][j] = mvwinch(plotwin, i, j) & A_CHARTEXT;
          }
          export_text(cmd_input + 2, buf, h, w);
          for (int i = 0; i < h; i++)
            free(buf[i]);
          free(buf);
        }
        cmd_input[0] = '\0';
        cmd_pos = 0;
        if (mode == mCOMMAND)
          mode = mNORMAL;
        redraw = 1;
        break;
      case 27:
        cmd_input[0] = '\0';
        cmd_pos = 0;
        mode = mNORMAL;
        redraw = 1;
        break;
      case KEY_BACKSPACE:
      case 127:
      case '\b':
        if (cmd_pos > 0)
          cmd_input[--cmd_pos] = '\0';
        else {
          mode = mNORMAL;
          cmd_input[0] = '\0';
        }
        redraw = 1;
        break;
      default:
        if (isprint(ch) && cmd_pos < mmFormulaLen - 1) {
          cmd_input[cmd_pos++] = ch;
          cmd_input[cmd_pos] = '\0';
          redraw = 1;
        }
        break;
      }
    } else if (mode == mINSERT) {
      if (funcs.count == 0) {
        mode = mNORMAL;
        continue;
      }
      char *formula = funcs.functions[funcs.sel].formula;
      int len = strlen(formula);
      switch (ch) {
      case 27:
        mode = mNORMAL;
        redraw = 1;
        break;
      case '\n':
      case KEY_ENTER:
        if (view.autoScale)
          autoscale(&view, &funcs);
        mode = mNORMAL;
        redraw = replot = 1;
        break;
      case KEY_BACKSPACE:
      case 127:
      case '\b':
        if (len > 0)
          formula[len - 1] = '\0';
        redraw = 1;
        break;
      default:
        if (isprint(ch) && len < mmFormulaLen - 1) {
          formula[len] = ch;
          formula[len + 1] = '\0';
          redraw = 1;
        }
        break;
      }
    } else {
      switch (ch) {
      case 'i':
      case 'I':
        mode = mINSERT;
        redraw = 1;
        break;
      case 't':
      case 'T':
        mode = mTRACE;
        trace_x = (view.mX + view.mmX) / 2.0;
        if (funcs.count > 0) {
          find_crit_points(funcs.functions[funcs.sel].formula, &view,
                           critical_points, &critical_point_count);
        }
        redraw = replot = 1;
        break;
      case ':':
        mode = mCOMMAND;
        cmd_input[0] = '\0';
        cmd_pos = 0;
        redraw = 1;
        break;
      case 'q':
      case 'Q':
        running = 0;
        break;
      case KEY_UP:
      case 'k':
        if (funcs.sel > 0) {
          funcs.sel--;
          redraw = replot = 1;
        }
        break;
      case KEY_DOWN:
      case 'j':
        if (funcs.sel < funcs.count - 1) {
          funcs.sel++;
          redraw = replot = 1;
        }
        break;
      case '+':
      case '=':
        zoom(&view, 0.8);
        redraw = replot = 1;
        break;
      case '-':
      case '_':
        zoom(&view, 1.25);
        redraw = replot = 1;
        break;
      case 'r':
      case 'R':
        view = default_view;
        view.autoScale = 1;
        autoscale(&view, &funcs);
        redraw = replot = 1;
        break;
      case 'a':
      case 'A':
        view.autoScale = !view.autoScale;
        if (view.autoScale)
          autoscale(&view, &funcs);
        redraw = replot = 1;
        break;
      }
    }

    if (redraw)
      d_sidebar(sidebar, &funcs, &view, mode, cmd_input, show_derivative,
                trace_x, trace_slope, &integ);
    if (replot)
      d_plot(plotwin, &funcs, &view, mode == mTRACE, trace_x, show_derivative,
             trace_slope, &integ);
  }

  delwin(sidebar);
  delwin(plotwin);
  endwin();
  return 0;
}

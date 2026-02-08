// Created by Unium on 06.02.26

#include "graph.h"
#include "parser.h"
#include "stb_image_write.h"
#include "types.h"
#include <math.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const CDef cmds[cmdCount] = {
    {"q", "q", "Quit mathplot"},
    {"quit", "quit", "Quit mathplot"},
    {"help", "help", "Show help"},
    {"integrate", "integrate", "Integration mode"},
    {"add", "add <expr>", "Add function #n"},
    {"remove", "remove <n>", "Remove function #n"},
    {"select", "select <n>", "Select function #n"},
    {"w", "w <file>", "Export as ASCII text"},
    {"wi", "wi <file>", "Export as PNG image"},
};

const CDef *g_cmds(void) { return cmds; }

int g_cmd_matches(const char *inp, const CDef **matches, int mm) {
  int c = 0;
  int len = strlen(inp);

  if (len == 0) {
    for (int i = 0; i < cmdCount && c < mm; i++) {
      matches[c++] = &cmds[i];
    }
    return c;
  }

  char c_part[64];
  int c_len = 0;
  for (int i = 0; i < len && inp[i] != ' ' && c_len < 63; i++) {
    c_part[c_len++] = inp[i];
  }
  c_part[c_len] = '\0';

  for (int i = 0; i < cmdCount && c < mm; i++) {
    if (strncmp(cmds[i].c, c_part, c_len) == 0) {
      matches[c++] = &cmds[i];
    }
  }
  return c;
}

void export_text(const char *f_name, char **buff, int h, int w) {
  FILE *f = fopen(f_name, "w");
  if (!f)
    return;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++)
      fputc(buff[y][x], f);
    fputc('\n', f);
  }
  fclose(f);
}

void export_png(const char *f_name, char **buff, int h, int w) {
  int char_w = 6, char_h = 12;
  int img_w = w * char_w;
  int img_h = h * char_h;
  unsigned char *ps = calloc(img_w * img_h * 3, 1);
  if (!ps)
    return;
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      char c = buff[y][x];
      unsigned char brightness = 30;
      if (c == '#' || c == '*' || c == 'O')
        brightness = 255;
      else if (c == '+' || c == '.')
        brightness = 200;
      else if (c == '-' || c == '|')
        brightness = 100;
      else if (c == ':')
        brightness = 150;
      for (int cy = 0; cy < char_h; cy++) {
        for (int cx = 0; cx < char_w; cx++) {
          int px = x * char_w + cx;
          int py = y * char_h + cy;
          int idx = (py * img_w + px) * 3;
          ps[idx] = brightness;
          ps[idx + 1] = c == '0' ? 255 : brightness;
          ps[idx + 2] = brightness;
        }
      }
    }
  }
  stbi_write_png(f_name, img_w, img_h, 3, ps, img_w * 3);
  free(ps);
}

void d_help(WINDOW *win) {
  int h, w;
  getmaxyx(win, h, w);
  werase(win);
  box(win, 0, 0);
  wattron(win, COLOR_PAIR(3) | A_BOLD);
  mvwprintw(win, 1, (w - 20) / 2, "=== HELP ===");
  wattroff(win, COLOR_PAIR(3) | A_BOLD);
  int y = 3;
  wattron(win, COLOR_PAIR(6) | A_BOLD);
  mvwprintw(win, y++, 2, "Basic Controls:");
  wattroff(win, COLOR_PAIR(6) | A_BOLD);
  mvwprintw(win, y++, 3, "i       - Insert mode (type formula)");
  mvwprintw(win, y++, 3, "t       - Trace mode (examine curve)");
  mvwprintw(win, y++, 3, ":       - Command mode");
  mvwprintw(win, y++, 3, "+/-     - Zoom in/out");
  mvwprintw(win, y++, 3, "r       - Reset view");
  mvwprintw(win, y++, 3, "q       - Quit");
  y++;
  wattron(win, COLOR_PAIR(6) | A_BOLD);
  mvwprintw(win, y++, 2, "Trace Mode (press t):");
  wattroff(win, COLOR_PAIR(6) | A_BOLD);
  mvwprintw(win, y++, 3, "Left/Right - Move cursor");
  mvwprintw(win, y++, 3, "d          - Toggle derivative/tangent");
  mvwprintw(win, y++, 3, "s          - Snap to nearest critical pt");
  mvwprintw(win, y++, 3, "n/p        - Next/prev critical point");
  y++;
  wattron(win, COLOR_PAIR(6) | A_BOLD);
  mvwprintw(win, y++, 2, "Commands (press :):");
  wattroff(win, COLOR_PAIR(6) | A_BOLD);
  mvwprintw(win, y++, 3, ":add <expr>  - Add function");
  mvwprintw(win, y++, 3, ":remove <n>  - Remove function n");
  mvwprintw(win, y++, 3, ":select <n>  - Select function n");
  mvwprintw(win, y++, 3, ":integrate   - Enter integration mode");
  mvwprintw(win, y++, 3, ":w <file>    - Export ASCII to file");
  mvwprintw(win, y++, 3, ":wi <file>   - Export PNG image");
  mvwprintw(win, y++, 3, ":help        - Show this help");
  mvwprintw(win, y++, 3, ":q or :quit  - Quit");
  y++;
  wattron(win, COLOR_PAIR(2) | A_BOLD);
  mvwprintw(win, h - 2, (w - 25) / 2, "Press any key to close");
  wattroff(win, COLOR_PAIR(2) | A_BOLD);
  wrefresh(win);
}

void d_sidebar(WINDOW *win, FLists *funcs, PView *v, Mode mode,
               const char *cmd_input, int show_deriv, double trace_X,
               double trace_slope, IntegrationState *integ) {
  int h, w;
  getmaxyx(win, h, w);
  (void)w; // kys
  werase(win);
  box(win, 0, 0);
  wattron(win, COLOR_PAIR(7) | A_BOLD);
  mvwprintw(win, 1, 2, "=================================");
  mvwprintw(win, 2, 2, "            mathplot             ");
  mvwprintw(win, 3, 2, "=================================");
  wattroff(win, COLOR_PAIR(7) | A_BOLD);

  wattron(win, COLOR_PAIR(6) | A_BOLD);
  mvwprintw(win, 5, 2, "Functions:");
  wattroff(win, COLOR_PAIR(6) | A_BOLD);
  for (int i = 0; i < funcs->count && i < 6; i++) {
    if (i == funcs->sel)
      wattron(win, A_REVERSE);
    wattron(win, COLOR_PAIR(funcs->functions[i].col));
    char disp[28];
    snprintf(disp, sizeof(disp), "%d. %s", i + 1, funcs->functions[i].formula);
    disp[27] = '\0';
    mvwprintw(win, 6 + i, 3, "%-30s", disp);
    wattroff(win, COLOR_PAIR(funcs->functions[i].col));
    if (i == funcs->sel)
      wattroff(win, A_REVERSE);
  }

  int info_Y = 13;
  wattron(win, COLOR_PAIR(6) | A_BOLD);
  mvwprintw(win, info_Y++, 2, "View:");
  wattroff(win, COLOR_PAIR(6) | A_BOLD);
  wattron(win, COLOR_PAIR(5));
  mvwprintw(win, info_Y++, 3, "x: [%.2f, %.2f]", v->mX, v->mmX);
  mvwprintw(win, info_Y++, 3, "y: [%.2f, %.2f]", v->mY, v->mmY);

  if (mode == mTRACE && show_deriv && !isnan(trace_slope)) {
    info_Y++;
    wattron(win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(win, info_Y++, 2, "Derivative:");
    wattroff(win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(win, info_Y++, 3, "f'(%.3f) = %.6f", trace_X, trace_slope);
  }

  if (integ->active && !isnan(integ->result)) {
    info_Y++;
    wattron(win, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(win, info_Y++, 2, "Integral:");
    wattroff(win, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(win, info_Y++, 3, "[%.2f, %.2f] = %.6f", integ->a, integ->b,
              integ->result);
  }

  int input_Y = h - 4;
  if (mode == mCOMMAND) {
    const CDef *matches[cmdCount];
    int m_count = g_cmd_matches(cmd_input, matches, 6);
    if (m_count > 0) {
      int bs_Y = input_Y - m_count - 2;
      if (bs_Y < 17)
        bs_Y = 17;

      wattron(win, COLOR_PAIR(7));
      mvwprintw(win, bs_Y, 2, "Suggestions:");
      wattron(win, COLOR_PAIR(7));

      for (int i = 0; i < m_count && bs_Y + 1 + i < input_Y - 1; i++) {
        if (i == 0) {
          wattron(win, COLOR_PAIR(2) | A_BOLD);
        } else {
          wattron(win, COLOR_PAIR(7));
        }

        char s[32];
        snprintf(s, sizeof(s), " %-12s %s", matches[i]->s, matches[i]->d);
        mvwprintw(win, bs_Y + 1 + i, 2, "%-30s", s);

        if (i == 0) {
          wattron(win, COLOR_PAIR(2) | A_BOLD);
        } else {
          wattron(win, COLOR_PAIR(7));
        }
      }
    }

    wattron(win, COLOR_PAIR(4) | A_BOLD);
    mvwprintw(win, input_Y, 2, ":%s_", cmd_input);
    wattroff(win, COLOR_PAIR(4) | A_BOLD);

    wattron(win, COLOR_PAIR(8));
    mvwprintw(win, input_Y + 1, 2, "TAB: complete ESC: cancel");
    wattron(win, COLOR_PAIR(8));
  } else if (mode == mTRACE) {
    wattron(win, COLOR_PAIR(2) | A_BOLD);
    mvwprintw(win, input_Y - 1, 2, "-- TRACE --");
    mvwprintw(win, input_Y, 2, "d:deriv h/l:move n/p:crit");
    wattroff(win, COLOR_PAIR(2) | A_BOLD);
  } else if (mode == mINTEGRATE) {
    wattron(win, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(win, input_Y - 1, 2, "-- INTEGRATE --");
    mvwprintw(win, input_Y, 2,
              integ->selStart ? "Set a: <- ->" : "Set b: <- ->");
    wattroff(win, COLOR_PAIR(5) | A_BOLD);
  } else if (mode == mINSERT) {
    wattron(win, COLOR_PAIR(3) | A_BOLD);
    mvwprintw(win, input_Y - 1, 2, "-- INSERT --");
    wattroff(win, COLOR_PAIR(3) | A_BOLD);
    if (funcs->count > 0) {
      mvwprintw(win, input_Y, 2, "f(x) = %s_",
                funcs->functions[funcs->sel].formula);
    }
  } else {
    wattron(win, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(win, input_Y - 1, 2, "-- NORMAL --");
    wattroff(win, COLOR_PAIR(5) | A_BOLD);
    mvwprintw(win, input_Y, 2, ":help for commands");
  }

  wrefresh(win);
}

void d_plot(WINDOW *win, FLists *funcs, PView *v, int trace_mode,
            double trace_X, int show_deriv, double trace_slope,
            IntegrationState *integ) {
  int height, width;
  getmaxyx(win, height, width);
  werase(win);
  box(win, 0, 0);
  if (funcs->count == 0) {
    wattron(win, COLOR_PAIR(8));
    mvwprintw(win, height / 2, (width - 30) / 2, "Enter a function to plot...");
    wattroff(win, COLOR_PAIR(8));
    wrefresh(win);
    return;
  }
  int plot_H = height - 4;
  int plot_W = width - 4;
  char **buff = malloc(plot_H * sizeof(char *));
  int **cols = malloc(plot_H * sizeof(int *));
  for (int i = 0; i < plot_H; i++) {
    buff[i] = malloc(plot_W);
    cols[i] = calloc(plot_W, sizeof(int));
    memset(buff[i], ' ', plot_W);
  }
  int zero_y = (int)((0 - v->mY) / (v->mmY - v->mY) * plot_H);
  int zero_x = (int)((0 - v->mX) / (v->mmX - v->mX) * plot_W);
  if (zero_y >= 0 && zero_y < plot_H) {
    for (int x = 0; x < plot_W; x++)
      buff[plot_H - 1 - zero_y][x] = '-';
  }
  if (zero_x >= 0 && zero_x < plot_W) {
    for (int y = 0; y < plot_H; y++)
      buff[y][zero_x] = '|';
  }
  if (zero_y >= 0 && zero_y < plot_H && zero_x >= 0 && zero_x < plot_W) {
    buff[plot_H - 1 - zero_y][zero_x] = '+';
  }
  if (integ->active && funcs->count > 0) {
    const char *formula = funcs->functions[funcs->sel].formula;
    double a_px = (integ->a - v->mX) / (v->mmX - v->mX) * plot_W;
    double b_px = (integ->b - v->mX) / (v->mmX - v->mX) * plot_W;
    int start_px = (int)fmin(a_px, b_px);
    int end_px = (int)fmax(a_px, b_px);
    if (start_px < 0)
      start_px = 0;
    if (end_px >= plot_W)
      end_px = plot_W - 1;

    for (int px = start_px; px <= end_px; px++) {
      double val_X = v->mX + (v->mmX - v->mX) * px / plot_W;
      double val_Y = p_eval(formula, val_X);
      if (isnan(val_Y) || isinf(val_Y))
        continue;
      int py = (int)((val_Y - v->mY) / (v->mmY - v->mY) * plot_H);
      int zero_py = (int)((0 - v->mY) / (v->mmY - v->mY) * plot_H);
      int start_Y = fmin(py, zero_py);
      int end_Y = fmax(py, zero_py);
      for (int fill_y = start_Y; fill_y <= end_Y; fill_y++) {
        if (fill_y >= 0 && fill_y < plot_H) {
          int buf_y = plot_H - 1 - fill_y;
          if (buff[buf_y][px] == ' ')
            buff[buf_y][px] = '.';
        }
      }
    }
  }
  for (int f = 0; f < funcs->count; f++) {
    if (!funcs->functions[f].active)
      continue;
    const char *formula = funcs->functions[f].formula;
    int color = funcs->functions[f].col;
    int samples = plot_W * 3;
    double prev_Y = NAN;
    int prev_py = -1;

    for (int i = 0; i < samples; i++) {
      double x = v->mX + (v->mmX - v->mX) * i / samples;
      double y = p_eval(formula, x);
      if (isnan(y) || isinf(y)) {
        prev_Y = NAN;
        prev_py = -1;
        continue;
      }

      int px = (int)((x - v->mX) / (v->mmX - v->mX) * plot_W);
      int py = (int)((y - v->mY) / (v->mmY - v->mY) * plot_H);

      if (px >= 0 && px < plot_W && py >= 0 && py < plot_H) {
        buff[plot_H - 1 - py][px] = '*';
        cols[plot_H - 1 - py][px] = color;
        if (!isnan(prev_Y) && prev_py >= 0) {
          int steps = abs(py - prev_py);
          if (steps > 1 && steps < 20) {
            for (int s = 1; s < steps; s++) {
              int interp_py = prev_py + (py - prev_py) * s / steps;
              if (interp_py >= 0 && interp_py < plot_H) {
                buff[plot_H - 1 - interp_py][px] = '+';
                cols[plot_H - 1 - interp_py][px] = color;
              }
            }
          }
        }
      }
      prev_Y = y;
      prev_py = py;
    }
  }
  if (trace_mode && show_deriv && !isnan(trace_slope) && funcs->count > 0) {
    double trace_Y = p_eval(funcs->functions[funcs->sel].formula, trace_X);
    if (!isnan(trace_Y)) {
      for (int px = 0; px < plot_W; px++) {
        double x = v->mX + (v->mmX - v->mX) * px / plot_W;
        double tang_Y = trace_Y + trace_slope * (x - trace_X);
        int py = (int)((tang_Y - v->mY) / (v->mmY - v->mY) * plot_H);
        if (py >= 0 && py < plot_H) {
          int buf_y = plot_H - 1 - py;
          if (buff[buf_y][px] == ' ' || buff[buf_y][px] == '-' ||
              buff[buf_y][px] == '|') {
            buff[buf_y][px] = ':';
            cols[buf_y][px] = 3;
          }
        }
      }
    }
  }
  if (trace_mode && funcs->count > 0) {
    double trace_Y = p_eval(funcs->functions[funcs->sel].formula, trace_X);
    if (!isnan(trace_Y) && !isinf(trace_Y)) {
      int trace_px = (int)((trace_X - v->mX) / (v->mmX - v->mX) * plot_W);
      int trace_py = (int)((trace_Y - v->mY) / (v->mmY - v->mY) * plot_H);
      if (trace_px >= 0 && trace_px < plot_W && trace_py >= 0 &&
          trace_py < plot_H) {
        buff[plot_H - 1 - trace_py][trace_px] = 'O';
        cols[plot_H - 1 - trace_py][trace_px] = 3;
      }
    }
  }
  for (int y = 0; y < plot_H; y++) {
    for (int x = 0; x < plot_W; x++) {
      char c = buff[y][x];
      int color = cols[y][x] ? cols[y][x] : 6;
      if (c == '-' || c == '|' || c == '+')
        color = 6;
      if (c != ' ')
        wattron(win, COLOR_PAIR(color) | (c == 'O' || c == '*' ? A_BOLD : 0));
      mvwaddch(win, y + 2, x + 2, c);
      if (c != ' ')
        wattroff(win, COLOR_PAIR(color) | (c == 'O' || c == '*' ? A_BOLD : 0));
    }
  }
  if (trace_mode && funcs->count > 0) {
    double trace_Y = p_eval(funcs->functions[funcs->sel].formula, trace_X);
    if (!isnan(trace_Y)) {
      wattron(win, COLOR_PAIR(3) | A_BOLD | A_REVERSE);
      mvwprintw(win, height - 1, (width - 32) / 2, " X: %.4f  Y: %.4f ",
                trace_X, trace_Y);
      wattroff(win, COLOR_PAIR(3) | A_BOLD | A_REVERSE);
    }
  }
  for (int i = 0; i < plot_H; i++) {
    free(buff[i]);
    free(cols[i]);
  }
  free(buff);
  free(cols);
  wrefresh(win);
}

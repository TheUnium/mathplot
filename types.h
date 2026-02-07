// Created by Unium on 06.02.26

#ifndef TYPES_H
#define TYPES_H

#define mmHistory 20
#define mmFormulaLen 256
#define mmFuncs 10
#define sidebarWidth 38

// H History
// F Function
// P Plot

typedef struct {
  char formula[mmFormulaLen];
  int col;
} HEntry;

typedef struct {
  HEntry entries[mmHistory];
  int count;
  int sel;
} H;

typedef struct {
  char formula[mmFormulaLen];
  int col;
  int active;
} F;

typedef struct {
  F functions[mmFuncs];
  int count;
  int sel;
} FLists;

typedef struct {
  int active;
  double a, b;
  double result;
  int selStart;
} IntegrationState;

typedef struct {
  double mX, mmX;
  double mY, mmY;
  int autoScale;
} PView;

typedef enum { mNORMAL, mINSERT, mCOMMAND, mTRACE, mINTEGRATE, mHELP } Mode;

#endif // !TYPES_H

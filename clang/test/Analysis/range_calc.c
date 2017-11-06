// RUN: %clang_analyze_cc1 -analyzer-checker=core,debug.ExprInspection -verify %s

void clang_analyzer_eval(int);

#define INT_MAX    ((signed int)((~0U)>>1))
#define INT_MIN    ((signed int)(~((~0U)>>1)))

void addInts(int X)
{
  if (X >= 10 && X <= 50) {
    int Y = X + 2;
    clang_analyzer_eval(Y >= 12 && Y <= 52); // expected-warning{{TRUE}}
  }
  
  if (X < 5) {
    int Y = X + 1;
    clang_analyzer_eval(Y < 6); // expected-warning{{TRUE}}
  }

  if (X >= 1000) {
    int Y = X + 1; // might overflow
    clang_analyzer_eval(Y >= 1001); // expected-warning{{UNKNOWN}}
    clang_analyzer_eval(Y == INT_MIN); // expected-warning{{UNKNOWN}}
	clang_analyzer_eval(Y == INT_MIN || Y >= 1001); // expected-warning{{TRUE}}
  }
}

void addU8(unsigned char X)
{
  if (X >= 10 && X <= 50) {
    unsigned char Y = X + 2;
    clang_analyzer_eval(Y >= 12 && Y <= 52); // expected-warning{{TRUE}}
  }
  
  if (X < 5) {
    unsigned char Y = X + 1;
    clang_analyzer_eval(Y < 6); // expected-warning{{TRUE}}
  }

  // TODO
  if (X >= 10 && X <= 50) {
    unsigned char Y = X + (-256); // truncation
    clang_analyzer_eval(Y >= 10 && Y <= 50); // expected-warning{{FALSE}}
  }

  // TODO
  if (X >= 10 && X <= 50) {
    unsigned char Y = X + 256; // truncation
    clang_analyzer_eval(Y >= 10 && Y <= 50); // expected-warning{{FALSE}} expected-warning{{UNKNOWN}}
  }

  // TODO
  if (X >= 100) {
    unsigned char Y = X + 1; // truncation
	clang_analyzer_eval(Y == 0); // expected-warning{{FALSE}}}}
	clang_analyzer_eval(Y >= 101); // expected-warning{{TRUE}}
    clang_analyzer_eval(Y == 0 || Y >= 101); // expected-warning{{TRUE}}
  }

  if (X >= 100) {
    unsigned short Y = X + 1;
    clang_analyzer_eval(Y >= 101 && Y <= 256); // expected-warning{{TRUE}}
  }
}

void sub1(int X)
{
  if (X >= 10 && X <= 50) {
    int Y = X - 2;
    clang_analyzer_eval(Y >= 8 && Y <= 48); // expected-warning{{TRUE}}
  }

  if (X >= 10 && X <= 50) {
    unsigned char Y = (unsigned int)X - 20; // truncation
    clang_analyzer_eval(Y <= 30 || Y >= 246); // expected-warning{{TRUE}}
  }

  // TODO
  if (X >= 10 && X <= 50) {
    unsigned char Y = (unsigned int)X - 256; // truncation
    clang_analyzer_eval(Y >= 10 && Y <= 50); // expected-warning{{FALSE}} expected-warning{{UNKNOWN}}
  }

  if (X < 5) {
    int Y = X - 1; // might overflow
    clang_analyzer_eval(Y < 4); // expected-warning{{UNKNOWN}}
    clang_analyzer_eval(Y == INT_MAX); // expected-warning{{UNKNOWN}}
	clang_analyzer_eval(Y == INT_MAX || Y < 4); // expected-warning{{TRUE}}
  }

  if (X >= 1000) {
    int Y = X - 1;
    clang_analyzer_eval(Y >= 999); // expected-warning{{TRUE}}
  }
}

void subU8(unsigned char X)
{
  if (X >= 10 && X <= 50) {
    unsigned char Y = X - 2;
    clang_analyzer_eval(Y >= 8 && Y <= 48); // expected-warning{{TRUE}}
  }
  
  if (X >= 100) {
    unsigned char Y = X - 1;
    clang_analyzer_eval(Y >= 99 && Y <= 254); // expected-warning{{TRUE}}
  }

  if (X < 5) {
    unsigned char Y = X - 1; // overflow
    clang_analyzer_eval(Y < 4 || Y == 255); // expected-warning{{TRUE}}
  }

  // TODO
  if (X >= 10 && X <= 50) {
    unsigned char Y = X - (-256); // truncation
    clang_analyzer_eval(Y >= 10 && Y <= 50); // expected-warning{{FALSE}} expected-warning{{UNKNOWN}}
  }

  // TODO
  if (X >= 10 && X <= 50) {
    unsigned char Y = X - 256; // truncation
    clang_analyzer_eval(Y >= 10 && Y <= 50); // expected-warning{{FALSE}}
  }

  if (X >= 100) {
    unsigned short Y = X + 1;
    clang_analyzer_eval(Y >= 101 && Y <= 256); // expected-warning{{TRUE}}
  }
}

void div(int X)
{
  if (X >= 10 && X <= 50) {
    int Y = X / 2;
    clang_analyzer_eval(Y >= 5); // expected-warning{{TRUE}}
    clang_analyzer_eval(Y <= 25); // expected-warning{{TRUE}}
  }
  
  // No overflows
}



// RUN: %clang_analyze_cc1 -analyzer-checker=alpha,core -verify %s

// Avoid false positive.
static char *allv[] = {"rpcgen", "-s", "udp", "-s", "tcp"};
static int allc = sizeof(allv) / sizeof(allv[0]);
static void falsePositive1(void) {
  int i;
  for (i = 1; i < allc; i++) {
    const char *p = allv[i]; // no-warning
    i++;
  }
}

// Detect division by zero.
static int zero = 0;
void truePositive1() {
  int x = 1000 / zero; // expected-warning{{Division by zero}}
}

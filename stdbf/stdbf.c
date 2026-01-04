#include "stdbf.h"

#ifdef _WIN32

void start(void) {
  extern int entry(void);
  entry();
}

void bfputchar(char c) {}

char bfgetchar() {}

char *bfcalloc(unsigned long long size) {};

void bffree(char *ptr) {};

#endif

#include <windows.h>

#include "bfrt.h"

void start(void) {
  extern int entry(void);
  entry();
  ExitProcess(0);
}

void bfputchar(char c) {
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD written;
  WriteFile(h, &c, 1, &written, NULL);
}

char bfgetchar() {
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD written;
  char c;
  ReadFile(h, &c, 1, &written, NULL);
  return c;
}

char *bfcalloc(unsigned long long size) {
  HANDLE heap = GetProcessHeap();
  void *p = HeapAlloc(heap, HEAP_ZERO_MEMORY, size);
  return (char *)p;
};

void bffree(char *ptr) { HeapFree(GetProcessHeap(), 0, ptr); };

#include <cstdint>
#include <cstdlib>

#include "bfrtjit.hpp"

namespace stdbfjit {
void bfputchar(char c) { putchar(c); }
char bfgetchar() { return getchar(); }
uint8_t *bfcalloc(size_t size) { return (uint8_t *)calloc(size, 1); };
void bffree(uint8_t *ptr) { free(ptr); };
} // namespace stdbfjit

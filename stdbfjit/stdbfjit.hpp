#pragma once
#include <cstddef>
#include <cstdint>
#include <stdio.h>

namespace stdbfjit {
extern "C" void bfputchar(char);
extern "C" char bfgetchar();
extern "C" uint8_t *bfcalloc(size_t size);
extern "C" void bffree(uint8_t *);
} // namespace stdbfjit

#include <stdint.h>

extern "C" uint64_t uv_get_available_memory(void) {
  return 0;
}

extern "C" uint64_t uv_get_constrained_memory(void) {
  return 0;
}

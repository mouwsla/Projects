#include "s21_string.h"

void *s21_memcpy(void *dest, const void *src, s21_size_t n) {
  char *char_dest = (char *)dest;
  const char *char_src = (const char *)src;
  s21_size_t k = 0;
  while (k < n) {
    *char_dest = *char_src;

    char_dest++;
    char_src++;
    k++;
  }
  return (void *)dest;
}

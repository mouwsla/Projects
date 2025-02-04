#include "s21_string.h"

char *s21_strncpy(char *dest, const char *src, s21_size_t n) {
  s21_size_t len = s21_strlen(src), i;
  if (n > len) {
    i = len + 1;
  } else {
    i = n;
  }
  for (s21_size_t j = 0; j < i; j++) {
    dest[j] = src[j];
  }
  return dest;
}
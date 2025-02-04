#include "s21_string.h"

char *s21_strncat(char *dest, const char *src, s21_size_t n) {
  int len = s21_strlen(dest);
  s21_size_t k = 0;
  for (int i = 0; src[i] != '\0' && k < n; i++) {
    dest[len + i] = src[i];
    k++;
  }
  dest[len + s21_strlen(src)] = '\0';

  return dest;
}
#include "s21_string.h"

void *s21_memset(void *str, int c, s21_size_t n) {
  char *temp = (char *)str;
  s21_size_t k = 0;

  while (k < n) {
    *temp = (char)c;
    temp++;
    k++;
  }

  return (void *)str;
}
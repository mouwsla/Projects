#include "s21_string.h"

void *s21_memchr(const void *str, int c, s21_size_t n) {
  s21_size_t i = 0;
  while ((*((char *)str)) != '\0' && (*((char *)str)) != c && i < n) {
    i++;
    str++;
  }
  if ((*((char *)str)) == c) {
    return (void *)str;
  } else {
    return S21_NULL;
  }
}
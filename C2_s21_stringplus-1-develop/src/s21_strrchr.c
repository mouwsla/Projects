#include "s21_string.h"

char *s21_strrchr(const char *str, int c) {
  if (str == S21_NULL) {
    return S21_NULL;
  }
  const char *last_pozition = S21_NULL;
  while (*str) {
    if (*str == (char)c) {
      last_pozition = str;
    }
    str++;
  }
  if (c == '\0') {
    return (char *)str;
  }
  return (char *)last_pozition;
}
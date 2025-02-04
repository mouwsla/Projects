#include "s21_string.h"

char *s21_strchr(const char *str, int c) {
  int i = 0;
  while (str[i] != '\0' && str[i] != c) {
    i++;
  }
  if (str[i] == c) {
    return (char *)str + i;
  } else {
    return S21_NULL;
  }
}
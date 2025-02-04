#include "s21_string.h"

void *s21_to_lower(const char *str) {
  if (str == S21_NULL) {
    return S21_NULL;
  }
  char *arr = (char *)calloc(s21_strlen(str) + 1, sizeof(char));
  if (arr == S21_NULL) {
    return S21_NULL;
  }

  char *start = arr;

  while (*str != '\0') {
    if (*str >= 'A' && *str <= 'Z') {
      *arr = *str + 32;
    } else {
      *arr = *str;
    }
    str++;
    arr++;
  }

  *arr = '\0';
  return (void *)start;
}
#include "s21_string.h"

int s21_strncmp(const char *str1, const char *str2, s21_size_t n) {
  int result = 0;
  s21_size_t k = 0;
  while (k < n) {
    if (*str1 == *str2) {
      result = 0;
    } else if (*str1 > *str2) {
      result = 1;
      break;
    } else {
      result = -1;
      break;
    }
    str1++;
    str2++;
    k++;
  }
  return result;
}
#include "s21_string.h"

int s21_memcmp(const void *str1, const void *str2, s21_size_t n) {
  int result = 0;
  s21_size_t k = 0;
  while (k < n) {
    if ((*((char *)str1)) == ((*((char *)str2)))) {
      result = 0;
    } else if ((*((char *)str1)) > ((*((char *)str2)))) {
      result = 1;
    } else {
      result = -1;
    }
    str1++;
    str2++;
    k++;
  }
  return result;
}
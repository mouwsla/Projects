#include "s21_string.h"

char *s21_strpbrk(const char *str1, const char *str2) {
  int flag = 0;
  char *output = S21_NULL;
  if (str1 == S21_NULL || str2 == S21_NULL) {
    return S21_NULL;
  }
  for (s21_size_t i = 0; i < s21_strlen(str1) && !flag; i++) {
    for (s21_size_t j = 0; j < s21_strlen(str2) && !flag; j++) {
      if (str1[i] == str2[j]) {
        output = (char *)str1 + i;
        flag = 1;
      }
    }
  }
  return output;
}
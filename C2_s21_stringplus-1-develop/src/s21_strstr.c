#include "s21_string.h"

char *s21_strstr(const char *haystack, const char *needle) {
  char *str = S21_NULL;
  int stop = 0;
  if (s21_strlen(needle) == 0) {
    str = (char *)haystack;
    stop = 1;
  } else if (s21_strlen(haystack) < s21_strlen(needle)) {
    stop = 1;
  }
  for (s21_size_t i = 0;
       i <= s21_strlen(haystack) - s21_strlen(needle) && !stop; i++) {
    if (s21_strncmp(haystack + i, needle, s21_strlen(needle)) == 0) {
      str = (char *)haystack + i;
      stop = 1;
    }
  }
  return str;
}
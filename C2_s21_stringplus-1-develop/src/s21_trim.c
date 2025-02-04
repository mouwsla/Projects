#include "s21_string.h"

void *s21_trim(const char *src, const char *trim_chars) {
  if (src == S21_NULL || trim_chars == S21_NULL) {
    return S21_NULL;
  }
  const char *start = src;
  const char *end = src + s21_strlen(src) - 1;

  while (*start != '\0' && s21_strchr(trim_chars, *start)) {
    start++;
  }
  while (end >= start && s21_strchr(trim_chars, *end)) {
    end--;
  }
  s21_size_t new_length = end - start + 1;

  char *result = (char *)calloc(new_length + 1, sizeof(char));
  if (result == S21_NULL) {
    return S21_NULL;
  }

  s21_strncpy(result, start, new_length);
  result[new_length] = '\0';

  return (void *)result;
}
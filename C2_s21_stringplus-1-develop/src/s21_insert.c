#include "s21_string.h"

void *s21_insert(const char *src, const char *str, s21_size_t start_index) {
  if (src == S21_NULL || str == S21_NULL) {
    return S21_NULL;
  }
  s21_size_t src_len = s21_strlen(src);
  s21_size_t str_len = s21_strlen(str);
  if (start_index > src_len) {
    return S21_NULL;
  }
  char *result = (char *)calloc(src_len + str_len + 1, sizeof(char));
  if (result == S21_NULL) {
    return S21_NULL;
  }
  for (s21_size_t i = 0; i < start_index; i++) {
    result[i] = src[i];
  }
  for (s21_size_t i = 0; i < str_len; i++) {
    result[start_index + i] = str[i];
  }
  for (s21_size_t i = start_index; i < src_len; i++) {
    result[str_len + i] = src[i];
  }
  result[src_len + str_len] = '\0';

  return (void *)result;
}

#include "s21_decimal.h"

int s21_from_decimal_to_int(s21_decimal src, int *dst) {
  if (dst == NULL) {
    return S21_ERROR_CONVERT;
  }
  int scale = s21_get_scale(&src);
  long long mantissa = (long long)src.bits[0] | ((long long)src.bits[1] << 32);

  for (int i = 0; i < scale; i++) {
    mantissa /= 10;
  }
  if (s21_get_sign(&src)) {
    mantissa = -mantissa;
  }
  if (mantissa > INT_MAX || mantissa < INT_MIN) {
    return S21_ERROR_CONVERT;
  }
  *dst = (int)mantissa;
  return S21_OK;
}

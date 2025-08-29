#include "s21_decimal.h"

int s21_from_int_to_decimal(int src, s21_decimal *dst) {
  if (dst == NULL) {
    return S21_ERROR_CONVERT;
  }
  dst->bits[0] = abs(src);
  dst->bits[2] = dst->bits[1] = 0;
  s21_set_scale(dst, 0);
  s21_set_sign(dst, src < 0);
  return S21_OK;
}

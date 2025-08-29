#include "s21_decimal.h"

int s21_div(s21_decimal value_1, s21_decimal value_2, s21_decimal *result) {
  if (!result) return S21_ERROR_CONVERT;
  if (s21_is_zero(&value_2)) return S21_DEV_BY_ZERO;
  *result = (s21_decimal){0};
  int sign_1 = s21_get_sign(&value_1);
  int sign_2 = s21_get_sign(&value_2);
  int result_sign = sign_1 ^ sign_2;
  s21_set_sign(&value_1, 0);
  s21_set_sign(&value_2, 0);
  if (s21_is_equal(value_1, value_2)) {
    result->bits[0] = 1;
    s21_set_sign(result, result_sign);
    return S21_OK;
  }
  int scale = 0;
  while (s21_is_less(value_1, value_2) && scale < 28) {
    if (!s21_mul_by_10(&value_1)) {
      scale++;
    } else {
      break;
    }
  }

  s21_decimal remainder = {0};
  s21_decimal quotient = {0};

  for (int i = 0; i < 96; i++) {
    s21_left_shift_2(&quotient, 1);
    s21_left_shift_2(&remainder, 1);
    if (s21_get_bit(value_1, 95 - i)) {
      s21_set_bit(&remainder, 0, 1);
    }
    if (s21_is_greater_or_equal(remainder, value_2)) {
      s21_sub(remainder, value_2, &remainder);
      s21_set_bit(&quotient, 0, 1);
    }
  }
  int additional_scale = 0;
  if (!s21_is_zero(&remainder)) {
    while (!s21_is_zero(&remainder) && (scale + additional_scale) < 28) {
      if (!s21_mul_by_10(&remainder) && !s21_mul_by_10(&quotient)) {
        additional_scale++;
      } else {
        break;
      }
    }
  }

  scale += additional_scale;
  *result = quotient;
  s21_set_sign(result, result_sign);
  s21_set_scale(result, scale);

  return S21_OK;
}
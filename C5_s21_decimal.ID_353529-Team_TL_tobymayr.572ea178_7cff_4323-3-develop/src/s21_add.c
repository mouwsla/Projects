#include "s21_decimal.h"

int s21_add(s21_decimal value_1, s21_decimal value_2, s21_decimal *result) {
  if (!result) return S21_ERROR_CONVERT;
  *result = (s21_decimal){0};

  if (s21_get_sign(&value_1) != s21_get_sign(&value_2)) {
    s21_negate(value_2, &value_2);
    return s21_sub(value_1, value_2, result);
  }
  if (!s21_normalize(&value_1, &value_2)) {
    return S21_ERROR_CONVERT;
  }

  uint64_t temp_sum = 0;
  uint32_t carry = 0;
  int result_sign = s21_get_sign(&value_1);
  int overflow = 0;
  int scale = s21_get_scale(&value_1);

  for (int i = 0; i < 3; i++) {
    temp_sum = (uint64_t)value_1.bits[i] + (uint64_t)value_2.bits[i] + carry;
    result->bits[i] = (uint32_t)(temp_sum & 0xFFFFFFFF);
    carry = (uint32_t)(temp_sum >> 32);
    if (i == 2 && carry) {
      overflow = 1;
    }
  }
  s21_set_scale(result, scale);
  s21_set_sign(result, result_sign);

  if (overflow ||
      (result->bits[0] == 0xFFFFFFFF && result->bits[1] == 0xFFFFFFFF &&
       result->bits[2] == 0xFFFFFFFF)) {
    return result_sign ? S21_NUMBER_MIN_INF : S21_NUMBER_INF;
  }
  return S21_OK;
}

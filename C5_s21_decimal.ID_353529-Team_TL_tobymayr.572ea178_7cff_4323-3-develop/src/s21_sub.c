#include "s21_decimal.h"

int s21_sub(s21_decimal value_1, s21_decimal value_2, s21_decimal *result) {
  if (!result) return S21_ERROR_CONVERT;
  *result = (s21_decimal){0};

  int sign1 = s21_get_sign(&value_1);
  int sign2 = s21_get_sign(&value_2);

  if (!s21_normalize(&value_1, &value_2)) {
    return S21_ERROR_CONVERT;
  }

  int target_scale = s21_get_scale(&value_1);

  if (sign1 != sign2) {
    s21_set_sign(&value_2, sign1);
    return s21_add(value_1, value_2, result);
  }

  int greater =
      (value_1.bits[2] > value_2.bits[2]) ||
      (value_1.bits[2] == value_2.bits[2] &&
       value_1.bits[1] > value_2.bits[1]) ||
      (value_1.bits[2] == value_2.bits[2] &&
       value_1.bits[1] == value_2.bits[1] && value_1.bits[0] > value_2.bits[0]);

  s21_decimal max = greater ? value_1 : value_2;
  s21_decimal min = greater ? value_2 : value_1;

  uint64_t temp_sub = 0;
  int borrow = 0;
  for (int i = 0; i < 3; i++) {
    if (max.bits[i] >= min.bits[i] + borrow) {
      temp_sub = (uint64_t)max.bits[i] - min.bits[i] - borrow;
      borrow = 0;
    } else {
      temp_sub = (uint64_t)max.bits[i] + 0x100000000 - min.bits[i] - borrow;
      borrow = 1;
    }
    result->bits[i] = (uint32_t)temp_sub;
  }

  if (greater) {
    s21_set_sign(result, sign1);
  } else {
    s21_set_sign(result, !sign1);
  }

  s21_set_scale(result, target_scale);

  return S21_OK;
}

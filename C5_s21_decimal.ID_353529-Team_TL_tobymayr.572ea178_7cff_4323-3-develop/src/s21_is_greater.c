#include "s21_decimal.h"

int s21_is_greater(s21_decimal value_1, s21_decimal value_2) {
  int sign1 = s21_get_sign(&value_1);
  int sign2 = s21_get_sign(&value_2);
  if (sign1 != sign2) {
    return (sign1 == 0) ? 1 : 0;
  }
  s21_decimal abs_val1 = s21_abs(value_1);
  s21_decimal abs_val2 = s21_abs(value_2);

  s21_normalize(&abs_val1, &abs_val2);

  for (int i = 2; i >= 0; i--) {
    if (abs_val1.bits[i] > abs_val2.bits[i]) {
      return (sign1 == 0) ? 1 : 0;
    } else if (abs_val1.bits[i] < abs_val2.bits[i]) {
      return (sign1 == 0) ? 0 : 1;
    }
  }
  return 0;
}
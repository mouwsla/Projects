#include "s21_decimal.h"

int s21_is_less(s21_decimal value1, s21_decimal value2) {
  int result = S21_FALSE;
  int sign1 = s21_get_sign(&value1);
  int sign2 = s21_get_sign(&value2);

  if (sign1 != sign2) {
    result = sign1 && !sign2;
  } else {
    s21_normalize(&value1, &value2);
    for (int i = 2; i >= 0 && result == S21_FALSE; i--) {
      if (value1.bits[i] != value2.bits[i]) {
        result = (value1.bits[i] < value2.bits[i]) ^ sign1;
      }
    }
  }

  return result;
}
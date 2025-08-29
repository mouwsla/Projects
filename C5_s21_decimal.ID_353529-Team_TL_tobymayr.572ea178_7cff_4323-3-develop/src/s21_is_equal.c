#include "s21_decimal.h"

int s21_is_equal(s21_decimal value1, s21_decimal value2) {
  if (!s21_is_less(value1, value2) && !s21_is_greater(value1, value2)) {
    return S21_TRUE;
  }
  return S21_FALSE;
}

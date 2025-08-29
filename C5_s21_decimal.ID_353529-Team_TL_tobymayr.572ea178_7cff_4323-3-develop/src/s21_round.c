#include "s21_decimal.h"

int s21_round(s21_decimal value, s21_decimal *result) {
  int code = S21_OK;
  if (!result) {
    code = S21_ERROR_CONVERT;
  } else {
    uint32_t dec_scale = s21_get_scale(&value);
    uint32_t remainder = 0;

    *result = value;

    for (unsigned int i = 0; i < dec_scale - 1; i++) {
      s21_div_by_ten(result);
    }
    remainder = s21_div_by_ten(result);
    ;
    if (remainder >= 5) {
      result->bits[0] = result->bits[0] + 1;
    }
    s21_set_scale(result, 0);
  }
  return code;
}
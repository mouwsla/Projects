#include "s21_decimal.h"

int s21_truncate(s21_decimal value, s21_decimal *result) {
  int code = S21_OK;
  if (!result) {
    code = S21_ERROR_CONVERT;
  } else {
    uint32_t dec_scale = s21_get_scale(&value);

    *result = value;

    for (unsigned int i = 0; i < dec_scale; i++) {
      s21_div_by_ten(result);
    }
    s21_set_scale(result, 0);
  }
  return code;
}
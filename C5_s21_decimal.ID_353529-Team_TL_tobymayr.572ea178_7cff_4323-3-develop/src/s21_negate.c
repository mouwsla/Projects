#include "s21_decimal.h"

int s21_negate(s21_decimal value, s21_decimal *result) {
  int code = S21_OK;
  if (!result) {
    code = S21_ERROR_CONVERT;
  } else {
    uint32_t dec_scale = s21_get_sign(&value);

    *result = value;

    s21_set_sign(result, !dec_scale);
  }
  return code;
}

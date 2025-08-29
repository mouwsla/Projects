#include "s21_decimal.h"

int s21_get_scale(const s21_decimal *decimal) {
  return (decimal->bits[3] >> 16) & 0xFF;
}

void s21_set_scale(s21_decimal *decimal, int scale) {
  decimal->bits[3] &= ~(0xFF << 16);
  decimal->bits[3] |= (scale & 0xFF) << 16;
}

int s21_get_sign(s21_decimal *decimal) {
  return ((unsigned int)decimal->bits[3] >> 31) & 1;
}

void s21_set_sign(s21_decimal *decimal, int sign) {
  if (sign) {
    decimal->bits[3] |= (1U << 31);
  } else {
    decimal->bits[3] &= ~(1U << 31);
  }
}
int s21_is_zero(s21_decimal *value) {
  return (value->bits[0] == 0 && value->bits[1] == 0 && value->bits[2] == 0);
}

int s21_mul_by_10(s21_decimal *value) {
  uint64_t temp = 0;
  uint64_t carry = 0;

  for (int i = 0; i < 3; i++) {
    temp = (uint64_t)value->bits[i] * 10 + carry;
    value->bits[i] = (uint32_t)(temp & 0xFFFFFFFF);
    carry = temp >> 32;
  }

  if (carry) {
    return 0;
  }

  int scale = (value->bits[3] >> 16) & 0xFF;
  if (scale < 28) {
    value->bits[3] &= ~(0xFF << 16);
    value->bits[3] |= (scale + 1) << 16;
  } else {
    return 0;
  }

  return 1;
}

int s21_normalize(s21_decimal *a, s21_decimal *b) {
  int scale1 = s21_get_scale(a);
  int scale2 = s21_get_scale(b);
  while (scale1 > scale2) {
    if (!s21_mul_by_10(b)) return 0;
    scale2++;
  }

  while (scale2 > scale1) {
    if (!s21_mul_by_10(a)) return 0;
    scale1++;
  }

  s21_set_scale(a, scale1);
  s21_set_scale(b, scale2);
  return 1;
}
int s21_div_by_ten(s21_decimal *value) {
  uint64_t temp = 0;
  uint64_t remainder = 0;
  for (int i = 2; i >= 0; i--) {
    temp = (uint64_t)(value->bits[i]) + (remainder << 32);
    value->bits[i] = (uint32_t)(temp / 10);
    remainder = temp % 10;
  }
  return (uint32_t)remainder;
}

void s21_set_bit(s21_decimal *value, unsigned bit, int set) {
  int word = bit / 32;
  int position = bit % 32;
  if (set) {
    value->bits[word] |= (1u << position);
  } else {
    value->bits[word] &= ~(1u << position);
  }
}

int s21_get_bit_last(s21_decimal value) {
  int bitLast = 95;
  for (; (bitLast >= 0) && (!s21_get_bit(value, bitLast));) bitLast -= 1;
  return bitLast;
}

void s21_left_shift_2(s21_decimal *value, int shift) {
  if (!(s21_get_bit_last(*value) + shift > 95)) {
    for (int y = 0; y < shift; y += 1) {
      int bitTransfer_1 = s21_get_bit(*value, 31);
      int bitTransfer_2 = s21_get_bit(*value, 63);
      for (int x = 0; x < 3; x += 1) value->bits[x] <<= 1;
      if (bitTransfer_1) s21_set_bit(value, 32, 1);
      if (bitTransfer_2) s21_set_bit(value, 64, 1);
    }
  }
}

int s21_get_bit(s21_decimal value, unsigned bit) {
  int word = bit / 32;
  int position = bit % 32;
  return (value.bits[word] >> position) & 1;
}

s21_decimal s21_abs(s21_decimal value) {
  value.bits[3] &= ~(1 << 31);
  return value;
}

void s21_parse_mul(s21_decimal_long tmp_val_1, s21_decimal_long tmp_val_2,
                   s21_decimal_long *tmp_res, s21_decimal value_1,
                   s21_decimal value_2, s21_decimal *result) {
  int sign_1 = s21_get_sign(&value_1);
  int sign_2 = s21_get_sign(&value_2);
  int scale_1 = s21_get_scale(&value_1);
  int scale_2 = s21_get_scale(&value_2);

  if (scale_1 >= scale_2) {
    s21_set_scale(result, (scale_1 + scale_2));
  } else {
    s21_set_scale(result, (scale_2 + scale_1));
  }

  if (sign_1 != sign_2) s21_set_bit_true(result, 127);
  s21_mul_modules(tmp_val_1, tmp_val_2, tmp_res);
}

int s21_round_result(s21_decimal *result, s21_decimal_long value) {
  s21_decimal_long temp_int = {0};
  int sign = s21_get_sign(result);
  int bit_depth = s21_bits_len(value), output = 0;
  int scale = s21_get_scale(result);
  memcpy(temp_int.bits, value.bits, sizeof(value.bits));
  s21_truncate_modules(&temp_int, scale);
  int bit_depth_truncate = s21_bits_len(temp_int);

  if (bit_depth && (bit_depth_truncate <= 96)) {
    s21_truncate_nulls(&value, result);
    if (s21_get_scale(result) == 1) {
      bit_depth = s21_rounding(&value, result, 96, 2);
    } else {
      bit_depth = s21_rounding(&value, result, 96, 0);
    }
  }
  if (sign) s21_set_bit_true(result, 127);
  if (bit_depth > 96 || bit_depth_truncate > 96) {
    memset(value.bits, 0, sizeof(value.bits));
    output = s21_get_sign(result) ? 2 : 1;
  }
  memcpy(result->bits, value.bits, (sizeof(uint32_t) * 3));

  return output;
}

void s21_set_bit_true(s21_decimal *value, int pos) {
  value->bits[pos / 32] |= (1 << (pos % 32));
}

void s21_set_n_bit_true(s21_decimal_long *value, int pos) {
  value->bits[pos / 32] |= (1 << (pos % 32));
}

void s21_mul_modules(s21_decimal_long value_1, s21_decimal_long value_2,
                     s21_decimal_long *result) {
  s21_decimal_long temp_1, temp_2;
  memcpy(temp_1.bits, value_1.bits, sizeof(value_1));
  memcpy(temp_2.bits, value_2.bits, sizeof(value_2));
  for (int t = 0; t < (7 * 32); t++) {
    if (temp_2.bits[0] & 1) {
      s21_add_modules(*result, temp_1, result);
    }
    s21_left_shift(&temp_1);
    s21_right_shift(&temp_2);
  }
}

void s21_add_modules(s21_decimal_long value_1, s21_decimal_long value_2,
                     s21_decimal_long *result) {
  int64_t temp = 0, carry = 0;
  s21_decimal_long tmp;
  memcpy(tmp.bits, value_1.bits, sizeof(value_1));
  for (int i = 0; i < 7; i++) {
    result->bits[i] = temp = (int64_t)tmp.bits[i] + value_2.bits[i] + carry;
    carry = temp >> 32;
  }
}

void s21_sub_modules(s21_decimal_long value_1, s21_decimal_long value_2,
                     s21_decimal_long *result) {
  s21_decimal_long temp;
  memcpy(temp.bits, value_2.bits, sizeof(value_2));
  s21_neg(&temp);
  s21_add_modules(value_1, temp, result);
}

int s21_bits_len(s21_decimal_long value) {
  int res = 0;
  s21_decimal_long null = {0};
  s21_decimal_long temp_value;
  memcpy(temp_value.bits, value.bits, sizeof(value.bits));

  while (!s21_is_equal_modules(temp_value, null)) {
    s21_right_shift(&temp_value);
    res++;
  }

  return res;
}
int s21_is_greater_modules(s21_decimal_long value_1, s21_decimal_long value_2) {
  int res = 0;

  for (int i = 6; i >= 0; i--) {
    if (value_1.bits[i] > value_2.bits[i]) {
      res = 1;
      break;
    } else if (value_1.bits[i] < value_2.bits[i]) {
      break;
    }
  }

  return res;
}

int s21_is_equal_modules(s21_decimal_long value_1, s21_decimal_long value_2) {
  return value_1.bits[6] == value_2.bits[6] &&
         value_1.bits[5] == value_2.bits[5] &&
         value_1.bits[4] == value_2.bits[4] &&
         value_1.bits[3] == value_2.bits[3] &&
         value_1.bits[2] == value_2.bits[2] &&
         value_1.bits[1] == value_2.bits[1] &&
         value_1.bits[0] == value_2.bits[0];
}

void s21_truncate_modules(s21_decimal_long *value, int scale) {
  s21_decimal_long value_const = {0}, temp = {0};
  value_const.bits[0] = 10;
  memcpy(temp.bits, value->bits, sizeof(value->bits));

  for (int i = 0; i < scale; i++) {
    memset(value->bits, 0, sizeof(value->bits));
    s21_div_modules(temp, value_const, value);
    memcpy(temp.bits, value->bits, sizeof(value->bits));
  }
}

void s21_truncate_nulls(s21_decimal_long *value, s21_decimal *result) {
  s21_decimal_long temp = {0}, ten = {0};
  ten.bits[0] = 10;
  int result_int = s21_get_scale(result);
  s21_mod_modules(*value, ten, &temp);
  while (!temp.bits[0] && result_int) {
    result_int--;
    s21_truncate_modules(value, 1);
    memset(temp.bits, 0, sizeof(temp.bits));
    s21_mod_modules(*value, ten, &temp);
  }
  s21_set_scale(result, result_int);
}

void s21_div_modules(s21_decimal_long value_1, s21_decimal_long value_2,
                     s21_decimal_long *result) {
  s21_decimal_long temp_1, temp_2;
  memcpy(temp_1.bits, value_1.bits, sizeof(value_1));
  while (s21_is_greater_or_equal_modules(temp_1, value_2)) {
    int i = 0;
    memcpy(temp_2.bits, value_2.bits, sizeof(value_2));
    while (
        s21_is_greater_or_equal_modules(temp_1, s21_left_shift_out(temp_2))) {
      i++;
      s21_left_shift(&temp_2);
    }
    s21_set_n_bit_true(result, i);
    s21_sub_modules(temp_1, temp_2, &temp_1);
  }
}

void s21_mod_modules(s21_decimal_long value_1, s21_decimal_long value_2,
                     s21_decimal_long *result) {
  s21_decimal_long res = {0};
  s21_div_modules(value_1, value_2, &res);
  s21_mul_modules(res, value_2, result);
  s21_sub_modules(value_1, *result, result);
}

s21_decimal_long s21_left_shift_out(s21_decimal_long value) {
  s21_decimal_long tmp;
  memcpy(tmp.bits, value.bits, sizeof(value));
  s21_left_shift(&tmp);
  return tmp;
}

int s21_is_greater_or_equal_modules(s21_decimal_long value_1,
                                    s21_decimal_long value_2) {
  return (s21_is_equal_modules(value_1, value_2) ||
          s21_is_greater_modules(value_1, value_2));
}

void s21_neg(s21_decimal_long *value) {
  int64_t temp = 0, carry = 1;
  for (int i = 0; i < 7; i++) value->bits[i] = ~value->bits[i];
  for (int t = 0; t < 7; t++) {
    value->bits[t] = temp = (int64_t)value->bits[t] + carry;
    carry = (temp & 4294967296) >> 32;
  }
}

int s21_rounding(s21_decimal_long *value, s21_decimal *result, int bits,
                 int method) {
  int bit_depth = s21_bits_len(*value);
  int sign = s21_get_sign(result);
  int scale = s21_get_scale(result);
  s21_decimal_long const_ten = {0}, const_one = {0};
  const_one.bits[0] = 1, const_ten.bits[0] = 10;
  s21_decimal_long temp_fractions = {0};
  memcpy(temp_fractions.bits, value->bits, sizeof(value->bits));

  while ((bit_depth > bits && scale) || scale > 28) {
    scale--;
    s21_decimal_long temp_res = {0};
    s21_mod_modules(temp_fractions, const_ten, &temp_res);
    s21_truncate_modules(value, 1);
    s21_truncate_modules(&temp_fractions, 1);
    bit_depth = s21_bits_len(*value);

    if (temp_res.bits[0] >= 5 && method == 0) {
      if (bit_depth <= bits && scale <= 28)
        s21_add_modules(*value, const_one, value);
    } else if (temp_res.bits[0] && method == -1) {
      if (sign && bit_depth <= bits && !scale)
        s21_add_modules(*value, const_one, value);
    } else if (temp_res.bits[0] >= 5 && method == 1) {
      if (bit_depth <= bits && !scale)
        s21_add_modules(*value, const_one, value);
    }
    if (temp_res.bits[0] >= 5 &&
        (!scale || (bit_depth <= bits && scale <= 28)) && method == 2) {
      temp_res.bits[0] = 0;
      s21_mod_modules(*value, const_ten, &temp_res);
      if (temp_res.bits[0] % 2) s21_add_modules(*value, const_one, value);
    }
    temp_res.bits[0] = 0;
    bit_depth = s21_bits_len(*value);
  }
  s21_set_scale(result, scale);

  return bit_depth;
}

void s21_right_shift(s21_decimal_long *value) {
  int64_t carry = 0, temp = 0;
  carry = (((int64_t)value->bits[6] & (1 << 31)) != 0);
  for (int i = 6; i >= 0; i--) {
    temp = (int64_t)value->bits[i] & 1;
    value->bits[i] >>= 1;
    value->bits[i] |= carry << 31;
    carry = temp;
  }
}
void s21_left_shift(s21_decimal_long *value) {
  int64_t carry = 0, temp = 0;
  for (int i = 0; i < 7; i++) {
    temp = ((int64_t)value->bits[i] & (1 << 31)) >> 31;
    value->bits[i] <<= 1;
    value->bits[i] |= carry;
    carry = temp;
  }
}

// ==========================testing===============================

void s21_set_the_scale(s21_decimal *value, int scale) {
  value->bits[3] = ((value->bits[3] + scale) << 16);
}

int s21_get_the_scale(s21_decimal value) {
  return ((value.bits[3] << 8) >> 24);
}
void s21_parse_dec(float tmp_src, float src, s21_decimal *dst) {
  int exp = s21_get_exponent(tmp_src);
  tmp_src /= powf(10, (float)exp);
  dst->bits[0] = (int)tmp_src;
  if ((dst->bits[0] % 10) >= 5) {
    dst->bits[0] = (dst->bits[0] / 10) + 1;
  } else {
    dst->bits[0] /= 10;
  }
  s21_decimal tmp = {{10, 0, 0, 0}};
  for (int i = 0; i < (exp + 1); i++) s21_mul(*dst, tmp, dst);
  if (src < 0) s21_set_bit_true(dst, 127);
}

int s21_rounding_fractions(s21_decimal *value, int bits, int method) {
  s21_decimal_long tmp_value = {0};
  int sign = s21_get_sign(value);
  memcpy(tmp_value.bits, value->bits, (sizeof(unsigned int) * 3));

  switch (method) {
    case -1:
      s21_truncate_nulls(&tmp_value, value);
      if (sign) s21_set_bit_true(value, 127);
      s21_rounding(&tmp_value, value, bits, method);
      break;
    case 0:
      s21_rounding(&tmp_value, value, bits, method);
      break;
    case 1:
      s21_truncate_nulls(&tmp_value, value);
      s21_rounding(&tmp_value, value, bits, method);
      break;
  }

  memcpy(value->bits, tmp_value.bits, (sizeof(unsigned int) * 3));
  if (sign) s21_set_bit_true(value, 127);

  return s21_bits_len(tmp_value);
}

int s21_parse_float(s21_decimal *tmp_src, float *dst, int sign) {
  int exp = 0;
  s21_decimal_long temp = {0};
  memcpy(temp.bits, tmp_src->bits, (sizeof(unsigned int) * 3));
  while (temp.bits[0]) {
    s21_truncate_modules(&temp, 1);
    exp++;
  }
  exp = exp - 8;
  memcpy(temp.bits, tmp_src->bits, (sizeof(unsigned int) * 3));
  s21_truncate_modules(&temp, exp);
  if ((temp.bits[0] % 10) >= 5) {
    temp.bits[0] = (temp.bits[0] / 10) + 1;
  } else {
    temp.bits[0] /= 10;
  }
  *dst = temp.bits[0];
  *dst *= powf(10, (float)(exp + 1));
  if (sign) *dst *= -1;

  return (exp + 1) ? 0 : 1;
}

int s21_get_exponent(float value) {
  int exp = 0;
  while (value >= 1) {
    value /= 10;
    exp++;
  }
  return (exp - 8);
}

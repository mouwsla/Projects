#include <check.h>

#include "../s21_decimal.h"

#define MAX_INT 2147483647   // 0b01111111111111111111111111111111
#define MAX_UINT 4294967295  // 0b11111111111111111111111111111111
#define MINUS 2147483648     // 0b10000000000000000000000000000000
#define EXPONENT_20 1310720  // 0b00000000000101000000000000000000
#define OneAt31 1 << 31
#define ZeroBits 0b00000000000000000000000000000000
#define MinusZeroBits 0b10000000000000000000000000000000
#define OneAt0 0b00000000000000000000000000000001
#define TenBits 0b00000000000000000000000000001010
#define OverloadBits 0b11111000000000000000000000000000
#define Bits3 0b00000000000000000000000011111111
#define MaxInt 0b11111111111111111111111111111111
#define MaxIntMinusOne 0b11111111111111111111111111111110

#define POSITIVE_NAN_MASK 0x7f800000
#define NEGATIVE_NAN_MASK 0xff800000
#define POSITIVE_ZERO_MASK 0x00000000
#define NEGATIVE_ZERO_MASK 0x80000000
#define TOO_SMALL 1e-29

/// =============== VARIABLES ===============
s21_decimal nol = {{0, 0, 0, 0}};
s21_decimal one = {{0, 0, 0, 0}};
s21_decimal one_minus = {{0, 0, 0, MINUS}};
s21_decimal max = {{MAX_UINT, MAX_UINT, MAX_UINT, 0}};
s21_decimal max_MAX = {{0b11111111111111111111111111111111,
                        0b11111111111111111111111111111111,
                        0b11111111111111111111111111111111, 0}};
s21_decimal min_MIN = {
    {0b11111111111111111111111111111111, 0b11111111111111111111111111111111,
     0b11111111111111111111111111111111, 0b10000000000000000000000000000000}};
s21_decimal middle = {{MAX_INT, MAX_INT, MAX_INT, 0}};
s21_decimal null = {{0, 0, 0, 0}};
s21_decimal null_minus = {{0, 0, 0, MINUS}};
s21_decimal middle_minus = {{MAX_INT, MAX_INT, MAX_INT, MINUS}};
s21_decimal max_minus = {{MAX_UINT, MAX_UINT, MAX_UINT, MINUS}};
s21_decimal middle_fractions = {{MAX_INT, MAX_INT, MAX_INT, EXPONENT_20}};
s21_decimal middle_fractions_minus = {
    {MAX_INT, MAX_INT, MAX_INT, (EXPONENT_20 | MINUS)}};
s21_decimal round_num = {{4560000, 0, 0, 327680}};
s21_decimal floor_num = {{0, 0, 0, MAX_INT}};
s21_decimal tmp = {{15, 0, 0, MINUS}};
s21_decimal tmp_2 = {{4, 0, 0, 0}};
s21_decimal tmp_float = {{214748413, 0, 0, 0b00000000000000010000000000000000}};

/// ===============   RESULTS  ===============
s21_decimal res_middle_add_middle_fractions = {
    {2543624459, 2147483647, 2147483647, 0}};
s21_decimal res_middle_add_middle = {{4294967294, 4294967294, 4294967294, 0}};
s21_decimal res_middle_minus_add_middle_minus = {
    {4294967294, 4294967294, 4294967294, MINUS}};
s21_decimal res_middle_fractions_add_middle_fractions = {
    {4294967294, 4294967294, 4294967294, EXPONENT_20}};
s21_decimal res_middle_add_max_minus = {
    {2147483648, 2147483648, 2147483648, MINUS}};
s21_decimal res_max_sub_middle = {{2147483648, 2147483648, 2147483648, 0}};
s21_decimal res_middle_fraction_sub_middle_fraction = {
    {20482339, 3303706602, 142724, 0}};
s21_decimal res_middle_fraction_mul_middle_fraction = {
    {3034176889, 3892122324, 850705916, 720896}};
s21_decimal res_middle_fraction_mul_middle_fraction_minus = {
    {3034176889, 3892122324, 850705916, (720896 | MINUS)}};

/// ========== CONVERSION & OTHER FUNCTIONS ===========
s21_decimal res_floor_middle_fractions = {{396140812, 0, 0, 0}};
s21_decimal res_floor_middle_fractions_minus = {{396140813, 0, 0, MINUS}};
s21_decimal res_round_middle_fractions_minus = {{396140812, 0, 0, MINUS}};
s21_decimal res_round = {{46, 0, 0, 0}};
s21_decimal res_floor = {{0, 0, 0, MINUS}};
s21_decimal res_from_float_to_decimal = {{21474840, 0, 0, 0}};

s21_decimal temp_res = {{0, 0, 0, 0}};

s21_decimal test1 = {{MaxInt, MaxInt, MaxInt, ZeroBits}};
s21_decimal res1 = {{MaxIntMinusOne, MaxInt, MaxInt, ZeroBits}};

s21_decimal test21 = {{MaxInt, ZeroBits, ZeroBits, ZeroBits}};
s21_decimal test22 = {{OneAt0, ZeroBits, ZeroBits, ZeroBits}};
s21_decimal res2 = {{ZeroBits, OneAt0, ZeroBits, ZeroBits}};

s21_decimal test31 = {
    {0b00000000000000000000010010110010, ZeroBits, ZeroBits, ZeroBits}};
s21_decimal test32 = {
    {0b00000000000000000000101001111101, ZeroBits, ZeroBits, ZeroBits}};
s21_decimal res3 = {
    {0b00000000000000000000111100101111, ZeroBits, ZeroBits, ZeroBits}};

s21_decimal test41 = {
    {0b00000000000000000000010010110010, ZeroBits, ZeroBits, OneAt31}};
s21_decimal test42 = {
    {0b00000000000000000000101001111101, ZeroBits, ZeroBits, OneAt31}};
s21_decimal res4 = {
    {0b00000000000000000000111100101111, ZeroBits, ZeroBits, OneAt31}};

s21_decimal test51 = {{0b01001111010111110111101100100111,
                       0b00000000000000000000000010010110, ZeroBits, ZeroBits}};
s21_decimal test52 = {
    {0b00000000000000000000101001111101, ZeroBits, ZeroBits, ZeroBits}};
s21_decimal res5 = {{0b01001111010111111000010110100100,
                     0b00000000000000000000000010010110, ZeroBits, ZeroBits}};

s21_decimal test61 = {{MaxInt, MaxInt, MaxInt, OneAt31}};
s21_decimal test62 = {{MaxInt, MaxInt, MaxInt, OneAt31}};
s21_decimal res6 = {{MaxIntMinusOne, MaxInt, MaxInt, OneAt31}};

s21_decimal test71 = {{0b00000000000000000000001000011111, ZeroBits, ZeroBits,
                       0b00000000000000010000000000000000}};
s21_decimal test72 = {{0b00000000000000000000000001111011, ZeroBits, ZeroBits,
                       0b00000000000000100000000000000000}};
s21_decimal res7 = {{0b00000000000000000001010110110001, ZeroBits, ZeroBits,
                     0b00000000000000100000000000000000}};

s21_decimal test81 = {{MaxInt, ZeroBits, ZeroBits, OneAt31}};
s21_decimal test82 = {{OneAt0, ZeroBits, ZeroBits, ZeroBits}};
s21_decimal res8 = {{MaxIntMinusOne, ZeroBits, ZeroBits, OneAt31}};

s21_decimal test91 = {{0b00000000000000000000001000011111, ZeroBits, ZeroBits,
                       0b00000000000000010000000000000000}};
s21_decimal test92 = {{0b00000000000000000000000001111011, ZeroBits, ZeroBits,
                       0b00000000000000100000000000000000}};
s21_decimal res9 = {{0b00000000000000000001010010111011, ZeroBits, ZeroBits,
                     0b00000000000000100000000000000000}};

s21_decimal test101 = {{500, 0, 0, 0}};
s21_decimal test102 = {{300, 0, 0, 1 << 31}};
s21_decimal res10 = {{800, 0, 0, 0}};

s21_decimal test111 = {{MaxInt, ZeroBits, ZeroBits, OneAt31}};
s21_decimal test112 = {{MaxInt, ZeroBits, ZeroBits, ZeroBits}};
s21_decimal res11 = {{4294967294, 1, 0, 2147483648}};

START_TEST(add) {
  int overload = s21_add(test1, test1, &temp_res);
  ck_assert_int_eq(1, s21_is_equal(res1, temp_res));
  ck_assert_int_eq(1, overload);

  overload = s21_add(test21, test22, &temp_res);
  ck_assert_int_eq(1, s21_is_equal(res2, temp_res));
  ck_assert_int_eq(0, overload);

  overload = s21_add(test31, test32, &temp_res);
  ck_assert_int_eq(1, s21_is_equal(res3, temp_res));
  ck_assert_int_eq(0, overload);

  overload = s21_add(test41, test42, &temp_res);
  ck_assert_int_eq(1, s21_is_equal(res4, temp_res));
  ck_assert_int_eq(0, overload);

  overload = s21_add(test51, test52, &temp_res);
  ck_assert_int_eq(1, s21_is_equal(res5, temp_res));
  ck_assert_int_eq(0, overload);

  overload = s21_add(test61, test62, &temp_res);
  ck_assert_int_eq(1, s21_is_equal(res6, temp_res));
  ck_assert_int_eq(2, overload);

  overload = s21_add(test71, test72, &temp_res);
  ck_assert_int_eq(1, s21_is_equal(res7, temp_res));
  ck_assert_int_eq(0, overload);

  s21_decimal val1 = {0}, val2 = {0}, res = {0}, expected = {0};
  int status;

  val1.bits[0] = 10;
  val2.bits[0] = 0;
  expected.bits[0] = 10;
  status = s21_add(val1, val2, &res);
  ck_assert_int_eq(status, S21_OK);
  ck_assert_int_eq(s21_is_equal(res, expected), 1);
  val1.bits[0] = 7;
  s21_set_sign(&val1, 1);
  val2.bits[0] = 2;
  s21_set_sign(&val2, 1);
  expected.bits[0] = 9;
  s21_set_sign(&expected, 1);
  status = s21_add(val1, val2, &res);
  ck_assert_int_eq(status, S21_OK);
  ck_assert_int_eq(s21_is_equal(res, expected), 1);
  val1.bits[0] = 10;
  s21_set_sign(&val1, 0);
  val2.bits[0] = 3;
  s21_set_sign(&val2, 1);
  expected.bits[0] = 7;
  s21_set_sign(&expected, 0);
  status = s21_add(val1, val2, &res);
  ck_assert_int_eq(status, S21_OK);
  ck_assert_int_eq(s21_is_equal(res, expected), 1);

  s21_decimal val111 = {{1000, 0, 0, 0}};
  s21_decimal val222 = {{1, 0, 0, 0}};
  s21_decimal res_1212 = {0};
  s21_decimal expected_122 = {{1001, 0, 0, 0}};

  s21_set_scale(&val111, 3);
  s21_set_scale(&val222, 3);
  s21_set_scale(&expected_122, 3);
  int status_1212 = s21_add(val111, val222, &res_1212);
  ck_assert_int_eq(status_1212, S21_OK);
  ck_assert_int_eq(s21_is_equal(res_1212, expected_122), 1);

  val1.bits[0] = 5;
  val2.bits[0] = 3;
  status = s21_add(val1, val2, NULL);
  ck_assert_int_eq(status, S21_ERROR_CONVERT);

  status = s21_add(max_MAX, one, &res);
  ck_assert_int_eq(status, S21_NUMBER_INF);
  status = s21_add(min_MIN, one_minus, &res);
  ck_assert_int_eq(status, S21_NUMBER_MIN_INF);
  s21_decimal decimal1 = {{0x122233EE, 0x5675EBE6, 0xD1C83484, 0x140000}};
  s21_decimal decimal2 = {{0x192F9AB, 0x0, 0x0, 0x80000000}};
  s21_decimal check = {{0x567233EE, 0x8A6A4F1A, 0xC93FABFE, 0x140000}};
  status = s21_add(decimal1, decimal2, &res);
  ck_assert_int_eq(status, S21_OK);
  ck_assert_int_eq(s21_is_equal(res, check), 1);

  decimal1 = (s21_decimal){{0x122233EE, 0x5675EBE6, 0xD1C83484, 0x140000}};
  decimal2 = (s21_decimal){{0x8, 0x0, 0x0, 0x0}};
  check = (s21_decimal){{0x2AA233EE, 0xB4B0DD51, 0xD1C834AF, 0x140000}};
  status = s21_add(decimal1, decimal2, &res);
  ck_assert_int_eq(status, S21_OK);
  ck_assert_int_eq(s21_is_equal(res, check), 1);

  decimal1 = (s21_decimal){{0x122233EE, 0x5675EBE6, 0xD1C83484, 0x140000}};
  decimal2 = (s21_decimal){{0x8, 0x0, 0x0, 0x80000000}};
  check = (s21_decimal){{0xF9A233EE, 0xF83AFA7A, 0xD1C83458, 0x140000}};
  status = s21_add(decimal1, decimal2, &res);
  ck_assert_int_eq(status, S21_OK);
  ck_assert_int_eq(s21_is_equal(res, check), 1);

  s21_decimal value_1 = {{5, 0, 0, 0}};
  s21_set_sign(&value_1, 1);
  s21_decimal value_2 = {{3, 0, 0, 0}};
  s21_decimal result = {0};
  status = s21_add(value_1, value_2, &result);
  ck_assert_uint_eq(result.bits[0], 2);
}
END_TEST

START_TEST(sub) {
  int overload = s21_sub(test81, test82, &temp_res);
  ck_assert_int_eq(0, s21_is_equal(res8, temp_res));
  ck_assert_int_eq(0, overload);
  overload = s21_sub(test91, test92, &temp_res);
  ck_assert_int_eq(1, s21_is_equal(res9, temp_res));
  ck_assert_int_eq(0, overload);

  overload = s21_sub(test101, test102, &temp_res);
  ck_assert_int_eq(1, s21_is_equal(res10, temp_res));
  ck_assert_int_eq(0, overload);

  overload = s21_sub(test111, test112, &temp_res);
  ck_assert_int_eq(1, s21_is_equal(res11, temp_res));
  ck_assert_int_eq(0, overload);
}
END_TEST

START_TEST(is_less) {
  ck_assert_int_eq(1, s21_is_less(middle, max));
  ck_assert_int_eq(0, s21_is_less(max, middle));
  ck_assert_int_eq(1, s21_is_less(middle_fractions, middle));
  ck_assert_int_eq(1, s21_is_less(middle_minus, middle));
  ck_assert_int_eq(1, s21_is_less(null_minus, null));
  ck_assert_int_eq(0, s21_is_less(null, null));
}
END_TEST

START_TEST(is_less_or_equal) {
  s21_decimal null = {0};
  s21_decimal null_minus = {0};
  s21_set_sign(&null_minus, 1);

  s21_decimal middle = {{12345, 0, 0, 0}};
  s21_decimal middle_minus = {{12345, 0, 0, 0}};
  s21_set_sign(&middle_minus, 1);

  s21_decimal middle_fractions = {{12345, 0, 0, 0}};
  s21_set_scale(&middle_fractions, 1);

  s21_decimal max_minus = {{UINT32_MAX, UINT32_MAX, UINT32_MAX, 0}};
  s21_set_sign(&max_minus, 1);

  ck_assert_int_eq(0, s21_is_less_or_equal(middle, null));
  ck_assert_int_eq(1, s21_is_less_or_equal(null, middle));

  ck_assert_int_eq(1, s21_is_less_or_equal(middle_minus, null));
  ck_assert_int_eq(1, s21_is_less_or_equal(max_minus, middle));

  ck_assert_int_eq(0, s21_is_less_or_equal(middle, middle_fractions));
  ck_assert_int_eq(1, s21_is_less_or_equal(middle_fractions, middle));

  ck_assert_int_eq(0, s21_is_less_or_equal(null, null_minus));
  ck_assert_int_eq(1, s21_is_less_or_equal(null_minus, null));

  s21_decimal middle_copy = middle;
  ck_assert_int_eq(1, s21_is_less_or_equal(middle, middle_copy));
  ck_assert_int_eq(1, s21_is_less_or_equal(middle_copy, middle));

  s21_decimal max = {{UINT32_MAX, UINT32_MAX, UINT32_MAX, 0}};
  ck_assert_int_eq(1, s21_is_less_or_equal(max, max));
  ck_assert_int_eq(1, s21_is_less_or_equal(null, null));
  ck_assert_int_eq(1, s21_is_less_or_equal(nol, nol));
}
END_TEST

START_TEST(is_greater) {
  ck_assert_int_eq(1, s21_is_greater(middle, null));
  ck_assert_int_eq(0, s21_is_greater(middle, max));
  ck_assert_int_eq(1, s21_is_greater(middle, max_minus));
  ck_assert_int_eq(1, s21_is_greater(max, middle_minus));
  ck_assert_int_eq(1, s21_is_greater(null, middle_minus));
  ck_assert_int_eq(0, s21_is_greater(middle, middle));
  ck_assert_int_eq(0, s21_is_greater(max, max));
  ck_assert_int_eq(0, s21_is_greater(max_MAX, max_MAX));
  ck_assert_int_eq(0, s21_is_greater(min_MIN, min_MIN));
  ck_assert_int_eq(0, s21_is_greater(nol, max_MAX));
  ck_assert_int_eq(1, s21_is_greater(max_MAX, nol));
}
END_TEST

START_TEST(is_greater_or_equal) {
  ck_assert_int_eq(1, s21_is_greater_or_equal(max, middle));
  ck_assert_int_eq(0, s21_is_greater_or_equal(middle, max));
  ck_assert_int_eq(1, s21_is_greater_or_equal(null, max_minus));
  ck_assert_int_eq(0, s21_is_greater_or_equal(middle_fractions, middle));
  ck_assert_int_eq(0, s21_is_greater_or_equal(null_minus, null));
  ck_assert_int_eq(1, s21_is_greater_or_equal(null, null));
}
END_TEST

START_TEST(is_equal) {
  ck_assert_int_eq(0, s21_is_equal(middle, max));
  ck_assert_int_eq(0, s21_is_equal(middle, middle_fractions));
  ck_assert_int_eq(0, s21_is_equal(null, null_minus));
  ck_assert_int_eq(1, s21_is_equal(null, null));
  ck_assert_int_eq(1, s21_is_equal(middle, middle));
}
END_TEST

START_TEST(is_not_equal) {
  ck_assert_int_eq(1, s21_is_not_equal(middle, middle_fractions));
  ck_assert_int_eq(1, s21_is_not_equal(middle, max));
  ck_assert_int_eq(1, s21_is_not_equal(null, null_minus));
  ck_assert_int_eq(0, s21_is_not_equal(null, null));
  ck_assert_int_eq(0, s21_is_not_equal(middle, middle));
}
END_TEST

START_TEST(mul) {
  int output = s21_mul(middle_fractions, middle_fractions, &temp_res);
  ck_assert_int_eq(
      1, s21_is_equal(res_middle_fraction_mul_middle_fraction, temp_res));
  ck_assert_int_eq(0, output);
  output = s21_mul(middle_fractions, null, &temp_res);
  ck_assert_int_eq(1, s21_is_equal(null, temp_res));
  ck_assert_int_eq(0, output);
  output = s21_mul(middle_fractions, middle_fractions_minus, &temp_res);
  ck_assert_int_eq(
      1, s21_is_equal(res_middle_fraction_mul_middle_fraction_minus, temp_res));
  ck_assert_int_eq(0, output);
  output = s21_mul(middle, middle, &temp_res);
  ck_assert_int_eq(1, s21_is_equal(null, temp_res));
  ck_assert_int_eq(1, output);
  output = s21_mul(middle_minus, middle, &temp_res);
  ck_assert_int_eq(1, s21_is_equal(null_minus, temp_res));
  ck_assert_int_eq(2, output);
}
END_TEST

START_TEST(test_div) {
  s21_decimal result = {0};
  int output;

  output = s21_div(middle, null, &result);
  ck_assert_int_eq(S21_DEV_BY_ZERO, output);

  output = s21_div(null, middle, &result);
  ck_assert_int_eq(1, s21_is_equal(null, result));
  ck_assert_int_eq(S21_OK, output);

  s21_decimal one = {{1, 0, 0, 0}};
  output = s21_div(middle, middle, &result);
  ck_assert_int_eq(1, s21_is_equal(one, result));
  ck_assert_int_eq(S21_OK, output);

  s21_decimal expected_neg_one = {{1, 0, 0, MINUS}};
  output = s21_div(middle, middle_minus, &result);
  ck_assert_int_eq(1, s21_is_equal(expected_neg_one, result));
  ck_assert_int_eq(S21_OK, output);
  s21_decimal small = {{2, 0, 0, 0}};
  output = s21_div(max, small, &result);

  s21_decimal two = {{2, 0, 0, 0}};
  output = s21_div(middle_fractions, two, &result);
  ck_assert_int_eq(1, s21_is_less(result, middle_fractions));
  ck_assert_int_eq(S21_OK, output);

  output = s21_div(middle, middle, NULL);
  ck_assert_int_eq(S21_ERROR_CONVERT, output);

  output = s21_div(tmp, tmp_2, &result);
  s21_decimal expected_neg_375 = {{375, 0, 0, (0x00020000 | MINUS)}};
  ck_assert_int_eq(
      1, s21_is_equal(expected_neg_375, result) || s21_is_less(result, null));
  ck_assert_int_eq(S21_OK, output);

  s21_decimal max_f = {{0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0}};
  s21_decimal two_f = {{2, 0, 0, 0}};
  s21_decimal result_f;
  int code_12 = s21_mul(max_f, two_f, &result_f);
  ck_assert_int_eq(code_12, S21_NUMBER_INF);

  s21_decimal max_mf = {{0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x80000000}};
  s21_decimal two_mf = {{2, 0, 0, 0}};
  s21_decimal result_mf;

  int code_21 = s21_mul(max_mf, two_mf, &result_mf);
  ck_assert_int_eq(code_21, S21_NUMBER_MIN_INF);

  s21_decimal max_11 = {{0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0}};
  s21_decimal one_11 = {{1, 0, 0, 0}};
  s21_decimal result_11;

  int code_52 = s21_add(max_11, one_11, &result_11);
  ck_assert_int_eq(code_52, S21_NUMBER_INF);
  s21_decimal val, val2, res, expect;
  int status;

  s21_from_int_to_decimal(5, &val);
  s21_from_int_to_decimal(0, &val2);
  ck_assert_int_eq(s21_div(val, val2, &res), S21_DEV_BY_ZERO);

  s21_from_int_to_decimal(1, &val);
  s21_from_int_to_decimal(1, &val2);
  status = s21_div(val, val2, &res);
  s21_from_int_to_decimal(1, &expect);
  ck_assert_int_eq(status, S21_OK);
  ck_assert_int_eq(s21_is_equal(res, expect), 1);

  s21_from_int_to_decimal(-10, &val);
  s21_from_int_to_decimal(5, &val2);
  status = s21_div(val, val2, &res);
  s21_from_int_to_decimal(-2, &expect);
  ck_assert_int_eq(status, S21_OK);
  ck_assert_int_eq(s21_is_equal(res, expect), 1);

  s21_from_int_to_decimal(1, &val);
  s21_from_int_to_decimal(3, &val2);
  status = s21_div(val, val2, &res);
  int si = s21_get_sign(&res);
  ck_assert_int_eq(status, S21_OK);
  ck_assert_int_eq(si, 0);

  val.bits[0] = 0xFFFFFFFF;
  val.bits[1] = 0xFFFFFFFF;
  val.bits[2] = 0xFFFFFFFF;
  val.bits[3] = 0;
  s21_decimal valqq = {{2, 0, 0, 0}};
  status = s21_div(val, valqq, &res);
  ck_assert_int_eq(status, S21_OK);

  ck_assert_int_eq(s21_div(val, val2, NULL), S21_ERROR_CONVERT);

  s21_decimal vallll = {0};
  vallll.bits[0] = 1;
  vallll.bits[3] = 6 << 16;
  s21_decimal val222 = {0};
  val222.bits[0] = 1;
  status = s21_div(vallll, val222, &res);
  ck_assert_int_eq(status, S21_OK);
  int sc = s21_get_scale(&res);
  printf("\n%d\n", sc);
  ck_assert_int_eq(sc, 0);
  s21_decimal val111 = {{2, 0, 0, ~(UINT_MAX / 2)}};
  s21_decimal val2222 = {{2, 0, 0, ~(UINT_MAX / 2)}};
  s21_decimal resss = {{0}};
  ck_assert_int_eq(0, s21_div(val111, val2222, &resss));
  s21_decimal val1a = {{2, 0, 0, 0}};
  s21_decimal val2a = {{0, 0, 0, 0}};
  s21_decimal resa = {{0}};
  ck_assert_int_eq(3, s21_div(val1a, val2a, &resa));
  s21_decimal val1aa = {{2, 0, 0, ~(UINT_MAX / 2)}};
  s21_decimal val2aa = {{2, 0, 0, ~(UINT_MAX / 2)}};
  s21_decimal resaa = {{0}};
  ck_assert_int_eq(0, s21_div(val1aa, val2aa, &resaa));
}
END_TEST

START_TEST(from_int_to_decimal_decimal_to_int) {
  int res = 0;
  int temp_plus = 2147483647;
  int temp_minus = -2147483647;
  int result = 0;
  int error = 0;
  s21_decimal dec = {0};
  int output = s21_from_int_to_decimal(temp_plus, &temp_res);
  ck_assert_int_eq(0, output);
  output = s21_from_decimal_to_int(temp_res, &res);
  ck_assert_int_eq(temp_plus, res);
  ck_assert_int_eq(0, output);
  output = s21_from_int_to_decimal(temp_minus, &temp_res);
  ck_assert_int_eq(0, output);
  output = s21_from_decimal_to_int(temp_res, &res);
  ck_assert_int_eq(temp_minus, res);
  ck_assert_int_eq(0, output);
  output = s21_from_decimal_to_int(temp_res, NULL);
  ck_assert_int_eq(temp_minus, res);
  ck_assert_int_eq(1, output);
  dec.bits[0] = 1234567890;
  s21_set_scale(&dec, 5);
  error = s21_from_decimal_to_int(dec, &result);
  ck_assert_int_eq(error, S21_OK);
  ck_assert_int_eq(result, 12345);
  dec.bits[0] = 987654;
  s21_set_scale(&dec, 3);
  s21_set_sign(&dec, 1);
  error = s21_from_decimal_to_int(dec, &result);
  ck_assert_int_eq(error, S21_OK);
  ck_assert_int_eq(result, -987);
  output = s21_from_decimal_to_int(temp_res, NULL);
  ck_assert_int_eq(1, output);
  output = s21_from_int_to_decimal(temp_plus, NULL);
  ck_assert_int_eq(1, output);
}
END_TEST

START_TEST(from_float_to_decimal_decimal_to_float) {
  float res = 0;
  float temp_plus = 2147483.647;
  float temp_minus = -2147483.647;
  float res_plus = 2147484.0;
  float res_minus = -2147484.0;

  int output = s21_from_float_to_decimal(temp_plus, &temp_res);
  ck_assert_int_eq(0, output);
  output = s21_from_decimal_to_float(temp_res, &res);
  ck_assert_float_eq(res_plus, res);
  ck_assert_int_eq(0, output);
  output = s21_from_float_to_decimal(temp_minus, &temp_res);
  ck_assert_int_eq(0, output);
  output = s21_from_decimal_to_float(temp_res, &res);
  ck_assert_float_eq(res_minus, res);
  ck_assert_int_eq(0, output);
  output = s21_from_float_to_decimal((temp_plus * 10), &temp_res);
  ck_assert_int_eq(1, s21_is_equal(res_from_float_to_decimal, temp_res));
  ck_assert_int_eq(0, output);
  output = s21_from_decimal_to_float(tmp_float, &res);
  ck_assert_float_eq(21474840.0, res);
  ck_assert_int_eq(0, output);
}
END_TEST

START_TEST(floor_) {
  int output = s21_floor(middle_fractions, &temp_res);
  ck_assert_int_eq(1, s21_is_equal(res_floor_middle_fractions, temp_res));
  ck_assert_int_eq(0, output);
  output = s21_floor(middle_fractions_minus, &temp_res);
  ck_assert_int_eq(1, s21_is_equal(res_floor_middle_fractions_minus, temp_res));
  ck_assert_int_eq(0, output);
  output = s21_floor(floor_num, &temp_res);
  ck_assert_int_eq(0, s21_is_equal(res_floor, temp_res));
  ck_assert_int_eq(0, output);
  output = s21_floor(middle_fractions, NULL);
  ck_assert_int_eq(0, s21_is_equal(res_floor_middle_fractions, temp_res));
  ck_assert_int_eq(1, output);
}
END_TEST

START_TEST(round_) {
  int output = s21_round(middle_fractions, &temp_res);
  ck_assert_int_eq(1, s21_is_equal(res_floor_middle_fractions, temp_res));
  ck_assert_int_eq(0, output);
  output = s21_round(middle_fractions_minus, &temp_res);
  ck_assert_int_eq(1, s21_is_equal(res_round_middle_fractions_minus, temp_res));
  ck_assert_int_eq(0, output);
  output = s21_round(round_num, &temp_res);
  ck_assert_int_eq(1, s21_is_equal(res_round, temp_res));
  ck_assert_int_eq(0, output);
  output = s21_round(middle_fractions, NULL);
  ck_assert_int_eq(0, s21_is_equal(res_floor_middle_fractions, temp_res));
  ck_assert_int_eq(1, output);
}
END_TEST

START_TEST(truncate) {
  int output = s21_truncate(middle_fractions, &temp_res);
  ck_assert_int_eq(1, s21_is_equal(res_floor_middle_fractions, temp_res));
  ck_assert_int_eq(0, output);
  output = s21_truncate(middle_fractions_minus, &temp_res);
  ck_assert_int_eq(1, s21_is_equal(res_round_middle_fractions_minus, temp_res));
  ck_assert_int_eq(0, output);
  output = s21_truncate(middle_fractions, NULL);
  ck_assert_int_eq(0, s21_is_equal(res_floor_middle_fractions, temp_res));
  ck_assert_int_eq(1, output);
}
END_TEST

START_TEST(negate) {
  int output = s21_negate(max_minus, &temp_res);
  ck_assert_int_eq(1, s21_is_equal(max, temp_res));
  ck_assert_int_eq(0, output);
  output = s21_negate(max, &temp_res);
  ck_assert_int_eq(1, s21_is_equal(max_minus, temp_res));
  ck_assert_int_eq(0, output);
  output = s21_negate(max_minus, NULL);
  ck_assert_int_eq(0, s21_is_equal(max, temp_res));
  ck_assert_int_eq(1, output);
}
END_TEST

int main() {
  Suite *s = suite_create("Suite");
  TCase *tc = tcase_create("Test case");
  SRunner *sr = srunner_create(s);
  int output;
  suite_add_tcase(s, tc);
  tcase_add_test(tc, add);
  tcase_add_test(tc, sub);
  tcase_add_test(tc, mul);
  tcase_add_test(tc, test_div);
  tcase_add_test(tc, floor_);
  tcase_add_test(tc, round_);
  tcase_add_test(tc, negate);
  tcase_add_test(tc, truncate);
  tcase_add_test(tc, is_less);
  tcase_add_test(tc, is_equal);
  tcase_add_test(tc, is_greater);
  tcase_add_test(tc, is_not_equal);
  tcase_add_test(tc, is_less_or_equal);
  tcase_add_test(tc, is_greater_or_equal);
  tcase_add_test(tc, from_int_to_decimal_decimal_to_int);
  tcase_add_test(tc, from_float_to_decimal_decimal_to_float);

  srunner_run_all(sr, CK_VERBOSE);
  output = srunner_ntests_failed(sr);
  srunner_free(sr);

  return output;
}

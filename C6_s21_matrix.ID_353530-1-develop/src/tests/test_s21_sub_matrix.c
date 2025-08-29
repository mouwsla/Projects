#include "test_s21_matrix.h"

START_TEST(sub_matrix) {
  const int rows = (int)get_rand(0, 1000) % 100 + 1;
  const int cols = (int)get_rand(0, 1000) % 100 + 1;
  matrix_t m = {0};
  s21_create_matrix(rows, cols, &m);
  matrix_t mtx = {0};
  s21_create_matrix(rows, cols, &mtx);
  matrix_t check = {0};
  s21_create_matrix(rows, cols, &check);
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      m.matrix[i][j] = get_rand(DBL_MIN, DBL_MAX);
      mtx.matrix[i][j] = get_rand(DBL_MIN, DBL_MAX);
      check.matrix[i][j] = m.matrix[i][j] - mtx.matrix[i][j];
    }
  }
  matrix_t res = {0};
  ck_assert_int_eq(s21_sub_matrix(&m, &mtx, &res), S21_OK);
  ck_assert_int_eq(s21_eq_matrix(&check, &res), SUCCESS);
  s21_remove_matrix(&m);
  s21_remove_matrix(&mtx);
  s21_remove_matrix(&res);
  s21_remove_matrix(&check);
}
END_TEST

START_TEST(test_incorrect) {
  matrix_t m = {0};
  matrix_t m2 = {0};
  matrix_t result;
  int code = s21_sum_matrix(&m, &m2, &result);
  ck_assert_int_eq(code, S21_INCORRECT_MATRIX);
}
END_TEST

START_TEST(sub_diff_size) {
  matrix_t m1;
  matrix_t m2;
  s21_create_matrix(2, 3, &m1);
  s21_create_matrix(3, 2, &m2);
  matrix_t result;
  ck_assert_int_eq(s21_sub_matrix(&m1, &m2, &result), S21_CALCULATION_ERROR);
  s21_remove_matrix(&result);
  s21_remove_matrix(&m1);
  s21_remove_matrix(&m2);
}
END_TEST

START_TEST(sub_single_element) {
  matrix_t m1;
  matrix_t m2;
  s21_create_matrix(1, 1, &m1);
  s21_create_matrix(1, 1, &m2);
  matrix_t check;
  s21_create_matrix(1, 1, &check);

  m1.matrix[0][0] = 5.5;
  m2.matrix[0][0] = 3.2;
  check.matrix[0][0] = 2.3;

  matrix_t result;
  ck_assert_int_eq(s21_sub_matrix(&m1, &m2, &result), S21_OK);
  ck_assert_int_eq(s21_eq_matrix(&check, &result), SUCCESS);

  s21_remove_matrix(&m1);
  s21_remove_matrix(&m2);
  s21_remove_matrix(&result);
  s21_remove_matrix(&check);
}
END_TEST

Suite *suite_s21_sub_matrix(void) {
  Suite *s = suite_create("suite_s21_sub_matrix");
  TCase *tc = tcase_create("s21_sub_matrix");
  tcase_add_loop_test(tc, sub_matrix, 0, 100);
  tcase_add_test(tc, test_incorrect);
  tcase_add_test(tc, sub_diff_size);
  tcase_add_test(tc, sub_single_element);
  suite_add_tcase(s, tc);
  return s;
}

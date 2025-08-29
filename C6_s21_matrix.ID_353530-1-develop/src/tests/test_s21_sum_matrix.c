#include "test_s21_matrix.h"

START_TEST(sum_matrix) {
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
      check.matrix[i][j] = m.matrix[i][j] + mtx.matrix[i][j];
    }
  }
  matrix_t res = {0};
  ck_assert_int_eq(s21_sum_matrix(&m, &mtx, &res), S21_OK);
  ck_assert_int_eq(s21_eq_matrix(&check, &res), SUCCESS);
  s21_remove_matrix(&m);
  s21_remove_matrix(&mtx);
  s21_remove_matrix(&res);
  s21_remove_matrix(&check);
}
END_TEST

START_TEST(test_incorrect) {
  matrix_t m = {0};
  matrix_t result = {0};
  int code = s21_sum_matrix(&m, &result, NULL);
  ck_assert_int_eq(code, S21_INCORRECT_MATRIX);
}
END_TEST

Suite *suite_s21_sum_matrix(void) {
  Suite *s = suite_create("suite_s21_sum_matrix");
  TCase *tc = tcase_create("s21_sum_matrix");
  tcase_add_loop_test(tc, sum_matrix, 0, 100);
  tcase_add_test(tc, test_incorrect);
  suite_add_tcase(s, tc);
  return s;
}

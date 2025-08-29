#include "test_s21_matrix.h"

START_TEST(test_create_normal_matrix) {
  const int rows = 5;
  const int cols = 5;
  matrix_t m = {0};

  int res = s21_create_matrix(rows, cols, &m);

  ck_assert_int_eq(res, S21_OK);
  ck_assert_int_eq(m.rows, rows);
  ck_assert_int_eq(m.columns, cols);
  ck_assert_ptr_nonnull(m.matrix);
  for (int i = 0; i < rows; i++) {
    for (int j = 0; j < cols; j++) {
      ck_assert_double_eq(m.matrix[i][j], 0.0);
    }
  }

  s21_remove_matrix(&m);
}
END_TEST

START_TEST(test_create_random_size_matrix) {
  const int rows = rand() % 100 + 1;
  const int cols = rand() % 100 + 1;
  matrix_t m = {0};

  int res = s21_create_matrix(rows, cols, &m);

  ck_assert_int_eq(res, S21_OK);
  ck_assert_int_eq(m.rows, rows);
  ck_assert_int_eq(m.columns, cols);
  ck_assert_ptr_nonnull(m.matrix);

  s21_remove_matrix(&m);
}
END_TEST

START_TEST(test_zero_rows) {
  const int rows = 0;
  const int cols = 5;
  matrix_t m = {0};

  int res = s21_create_matrix(rows, cols, &m);

  ck_assert_int_eq(res, S21_INCORRECT_MATRIX);
  ck_assert_int_eq(m.rows, 0);
  ck_assert_int_eq(m.columns, 0);
  ck_assert_ptr_null(m.matrix);
}
END_TEST

START_TEST(test_zero_cols) {
  const int rows = 5;
  const int cols = 0;
  matrix_t m = {0};

  int res = s21_create_matrix(rows, cols, &m);

  ck_assert_int_eq(res, S21_INCORRECT_MATRIX);
  ck_assert_int_eq(m.rows, 0);
  ck_assert_int_eq(m.columns, 0);
  ck_assert_ptr_null(m.matrix);
}
END_TEST

START_TEST(test_negative_rows) {
  const int rows = -5;
  const int cols = 5;
  matrix_t m = {0};

  int res = s21_create_matrix(rows, cols, &m);

  ck_assert_int_eq(res, S21_INCORRECT_MATRIX);
  ck_assert_int_eq(m.rows, 0);
  ck_assert_int_eq(m.columns, 0);
  ck_assert_ptr_null(m.matrix);
}
END_TEST

START_TEST(test_negative_cols) {
  const int rows = 5;
  const int cols = -5;
  matrix_t m = {0};

  int res = s21_create_matrix(rows, cols, &m);

  ck_assert_int_eq(res, S21_INCORRECT_MATRIX);
  ck_assert_int_eq(m.rows, 0);
  ck_assert_int_eq(m.columns, 0);
  ck_assert_ptr_null(m.matrix);
}
END_TEST

START_TEST(test_null_result_pointer) {
  const int rows = 5;
  const int cols = 5;

  int res = s21_create_matrix(rows, cols, NULL);

  ck_assert_int_eq(res, S21_INCORRECT_MATRIX);
}
END_TEST

Suite *suite_s21_create_matrix(void) {
  Suite *s = suite_create("s21_create_matrix");
  TCase *tc = tcase_create("Core");
  tcase_add_test(tc, test_create_normal_matrix);
  tcase_add_test(tc, test_create_random_size_matrix);
  tcase_add_test(tc, test_zero_rows);
  tcase_add_test(tc, test_zero_cols);
  tcase_add_test(tc, test_negative_rows);
  tcase_add_test(tc, test_negative_cols);
  tcase_add_test(tc, test_null_result_pointer);
  tcase_add_loop_test(tc, test_create_random_size_matrix, 0, 100);

  suite_add_tcase(s, tc);
  return s;
}

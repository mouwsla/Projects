#include "s21_matrix.h"

int s21_calc_complements(matrix_t *A, matrix_t *result) {
  int flag = S21_OK;
  if (!s21_is_correct_matrix(A)) flag = S21_INCORRECT_MATRIX;
  if (A->columns != A->rows && !flag) flag = S21_CALCULATION_ERROR;
  if (!flag && s21_create_matrix(A->columns, A->rows, result) != S21_OK)
    flag = S21_MALLOC_FAILED;
  if (!flag && A->rows == 1) result->matrix[0][0] = A->matrix[0][0];
  if (!flag) {
    matrix_t tmp;
    for (int i = 0; i < A->rows; i++) {
      for (int j = 0; j < A->columns; j++) {
        s21_create_matrix(A->rows - 1, A->columns - 1, &tmp);
        s21_new_mtx(i, j, A, &tmp);
        double tmp_rez = 0;
        s21_determinant(&tmp, &tmp_rez);
        result->matrix[i][j] = tmp_rez;
        s21_remove_matrix(&tmp);
      }
    }
    s21_rez_algebraic_complements(result);
  }
  return flag;
}
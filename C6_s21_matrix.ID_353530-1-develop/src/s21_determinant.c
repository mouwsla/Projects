#include "s21_matrix.h"

int s21_determinant(matrix_t *A, double *result) {
  *result = 0;
  int flag = S21_OK;
  if (!s21_is_correct_matrix(A) || !result) flag = S21_INCORRECT_MATRIX;
  if (A->rows != A->columns && !flag) flag = S21_CALCULATION_ERROR;
  if (!flag) {
    if (A->rows == 1) *result = A->matrix[0][0];
    if (A->rows == 2)
      *result =
          A->matrix[0][0] * A->matrix[1][1] - A->matrix[1][0] * A->matrix[0][1];
    if (A->rows > 2) {
      for (int i = 0; i < A->rows; i++) {
        matrix_t tmp;
        s21_create_matrix(A->rows - 1, A->columns - 1, &tmp);
        s21_new_mtx(i, 0, A, &tmp);
        double tmp_rez = 0;
        s21_determinant(&tmp, &tmp_rez);
        *result = *result + (A->matrix[i][0] * tmp_rez) * pow(-1, i);
        s21_remove_matrix(&tmp);
      }
    }
  }
  return flag;
}
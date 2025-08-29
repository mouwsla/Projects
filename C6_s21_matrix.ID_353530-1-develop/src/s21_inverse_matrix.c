#include "s21_matrix.h"

int s21_inverse_matrix(matrix_t *A, matrix_t *result) {
  int flag = S21_OK;
  if (!s21_is_correct_matrix(A)) flag = S21_INCORRECT_MATRIX;
  if (A->rows != A->columns && !flag) flag = S21_CALCULATION_ERROR;
  double det = 0;
  int code = s21_determinant(A, &det);
  if ((code || fabs(det) < 1e-7) && !flag) flag = S21_CALCULATION_ERROR;
  if (!flag) {
    if (A->rows == 1) {
      s21_create_matrix(1, 1, result);
      result->matrix[0][0] = 1 / A->matrix[0][0];
    } else {
      matrix_t transponir_mtx;
      matrix_t minor_mtx;
      s21_transpose(A, &transponir_mtx);
      s21_calc_complements(&transponir_mtx, &minor_mtx);
      det = 1 / det;
      s21_mult_number(&minor_mtx, det, result);
      s21_remove_matrix(&transponir_mtx);
      s21_remove_matrix(&minor_mtx);
    }
  }
  return flag;
}
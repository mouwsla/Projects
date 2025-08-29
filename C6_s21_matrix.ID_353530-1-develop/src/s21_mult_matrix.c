#include "s21_matrix.h"

int s21_mult_matrix(matrix_t *A, matrix_t *B, matrix_t *result) {
  int flag = S21_OK;
  if (!s21_is_correct_matrix(A) || !s21_is_correct_matrix(B))
    flag = S21_INCORRECT_MATRIX;
  if (A->columns != B->rows && !flag) flag = S21_CALCULATION_ERROR;
  if (s21_create_matrix(A->rows, B->columns, result) && !flag)
    flag = S21_MALLOC_FAILED;
  if (!flag) {
    for (int i = 0; i < A->rows; i++) {
      for (int j = 0; j < B->columns; j++) {
        double sum = 0;
        for (int x = 0; x < A->columns; x++) {
          sum += A->matrix[i][x] * B->matrix[x][j];
        }
        if (isinf(fabs(sum)) || isnan(fabs(sum))) flag = S21_CALCULATION_ERROR;
        if (!flag) result->matrix[i][j] = sum;
      }
    }
  }
  return flag;
}
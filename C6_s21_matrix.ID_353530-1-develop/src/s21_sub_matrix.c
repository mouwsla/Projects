#include "s21_matrix.h"

int s21_sub_matrix(matrix_t *A, matrix_t *B, matrix_t *result) {
  int flag = S21_OK;
  if (!s21_is_correct_matrix(A) || !s21_is_correct_matrix(B))
    flag = S21_INCORRECT_MATRIX;
  if ((A->columns != B->columns || A->rows != B->rows) && !flag)
    flag = S21_CALCULATION_ERROR;
  if (s21_create_matrix(A->rows, A->columns, result) && !flag)
    flag = S21_MALLOC_FAILED;
  if (!flag) {
    for (int i = 0; i < A->rows; i++) {
      for (int j = 0; j < A->columns; j++) {
        result->matrix[i][j] = A->matrix[i][j] - B->matrix[i][j];
      }
    }
  }
  return flag;
}

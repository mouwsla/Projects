#include "s21_matrix.h"

int s21_transpose(matrix_t *A, matrix_t *result) {
  int flag = S21_OK;
  if (!s21_is_correct_matrix(A)) flag = S21_INCORRECT_MATRIX;
  if (s21_create_matrix(A->columns, A->rows, result) && !flag)
    flag = S21_MALLOC_FAILED;
  if (!flag) {
    for (int i = 0; i < A->rows; i++) {
      for (int j = 0; j < A->columns; j++) {
        result->matrix[j][i] = A->matrix[i][j];
      }
    }
  }
  return flag;
}
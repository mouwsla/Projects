#include "s21_matrix.h"

int s21_mult_number(matrix_t *A, double number, matrix_t *result) {
  int flag = S21_OK;
  if (!s21_is_correct_matrix(A)) flag = S21_INCORRECT_MATRIX;
  if (s21_create_matrix(A->rows, A->columns, result) && !flag)
    flag = S21_MALLOC_FAILED;
  if (!flag) {
    for (int i = 0; i < A->rows; i++) {
      for (int j = 0; j < A->columns; j++) {
        result->matrix[i][j] = A->matrix[i][j] * number;
      }
    }
  }
  return flag;
}
#include "s21_matrix.h"

int s21_create_matrix(int rows, int columns, matrix_t *result) {
  int flag = S21_OK;
  if (rows <= 0 || columns <= 0 || !result) flag = S21_INCORRECT_MATRIX;
  if (!flag) {
    result->columns = columns;
    result->rows = rows;
    result->matrix = calloc(rows, sizeof(double *));
    if (!result->matrix) flag = S21_MALLOC_FAILED;
    for (int i = 0; i < rows; i++) {
      result->matrix[i] = calloc(columns, sizeof(double));
      if (!result->matrix[i]) {
        for (int j = 0; j < i; j++) free(result->matrix[j]);
        free(result->matrix);
        return S21_MALLOC_FAILED;
      }
    }
  }
  return flag;
}
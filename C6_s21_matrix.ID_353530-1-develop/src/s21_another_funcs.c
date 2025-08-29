#include "s21_matrix.h"

int s21_is_correct_matrix(matrix_t *A) {
  int flag = 1;
  if (!A || !A->matrix) flag = 0;
  for (int i = 0; i < A->rows; i++) {
    if (!A->matrix[i]) flag = 0;
  }
  if (flag && (A->rows <= 0 || A->columns <= 0)) flag = 0;
  return flag;
}

void s21_new_mtx(int skip_rows, int skip_columns, matrix_t *A, matrix_t *tmp) {
  int ix = 0, jx = 0;
  for (int i = 0; i < A->rows; i++) {
    if (i != skip_rows) {
      for (int j = 0; j < A->columns; j++) {
        if (j != skip_columns) {
          tmp->matrix[ix][jx] = A->matrix[i][j];
          jx++;
        }
      }
      ix++;
      jx = 0;
    }
  }
}
void s21_rez_algebraic_complements(matrix_t *A) {
  for (int i = 0; i < A->rows; i++) {
    for (int j = 0; j < A->columns; j++) {
      A->matrix[i][j] = A->matrix[i][j] * pow(-1, i + j);
    }
  }
}

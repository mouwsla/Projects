#include "s21_matrix_oop.h"

S21Matrix S21Matrix::operator+(const S21Matrix &other) const {
  S21Matrix result = *this;
  result.SumMatrix(other);
  return result;
}

S21Matrix S21Matrix::operator-(const S21Matrix &other) const {
  S21Matrix result = *this;
  result.SubMatrix(other);
  return result;
}
S21Matrix S21Matrix::operator*(const S21Matrix &other) const {
  S21Matrix result = *this;
  result.MulMatrix(other);
  return result;
}
bool S21Matrix::operator==(const S21Matrix &other) const {
  return this->EqMatrix(other);
}

S21Matrix &S21Matrix::operator=(const S21Matrix &other) {
  if (*this == other) {
    return *this;
  } else {
    this->~S21Matrix();
    this->cols_ = other.cols_;
    this->rows_ = other.rows_;
    this->matrix_ = new double *[this->rows_];
    for (int i = 0; i < this->rows_; i++) {
      this->matrix_[i] = new double[this->cols_];
      for (int j = 0; j < this->cols_; j++) {
        this->matrix_[i][j] = other.matrix_[i][j];
      }
    }
  }
  return *this;
}

S21Matrix &S21Matrix::operator+=(const S21Matrix &other) {
  this->SumMatrix(other);
  return *this;
}
S21Matrix &S21Matrix::operator-=(const S21Matrix &other) {
  this->SubMatrix(other);
  return *this;
}
S21Matrix &S21Matrix::operator*=(const S21Matrix &other) {
  this->MulMatrix(other);
  return *this;
}
S21Matrix &S21Matrix::operator*=(const double num) {
  this->MulNumber(num);
  return *this;
}

double &S21Matrix::operator()(const int row, const int col) const {
  if (row < 0 || col < 0 || row >= this->rows_ || col >= this->cols_) {
    throw out_of_range("operator(): Out of range.");
  }
  return this->matrix_[row][col];
}

#include "s21_matrix_oop.h"

bool S21Matrix::EqMatrix(const S21Matrix& other) const {
  if (this->cols_ != other.cols_ || this->rows_ != other.rows_) {
    return false;
  } else {
    for (int i = 0; i < this->rows_; i++) {
      for (int j = 0; j < this->cols_; j++) {
        if (this->matrix_[i][j] != other.matrix_[i][j]) {
          return false;
        }
      }
    }
    return true;
  }
}
void S21Matrix::SumMatrix(const S21Matrix& other) {
  if (this->rows_ != other.rows_ || this->cols_ != other.cols_) {
    throw out_of_range("SumMatrix: Invalid size of matrix.");
  } else {
    for (int i = 0; i < this->rows_; i++) {
      for (int j = 0; j < this->cols_; j++) {
        this->matrix_[i][j] += other.matrix_[i][j];
      }
    }
  }
}
void S21Matrix::SubMatrix(const S21Matrix& other) {
  if (this->rows_ != other.rows_ || this->cols_ != other.cols_) {
    throw out_of_range("SubMatrix: Invalid size of matrix.");
  } else {
    for (int i = 0; i < this->rows_; i++) {
      for (int j = 0; j < this->cols_; j++) {
        this->matrix_[i][j] -= other.matrix_[i][j];
      }
    }
  }
}
void S21Matrix::MulNumber(const double num) {
  for (int i = 0; i < this->rows_; i++) {
    for (int j = 0; j < this->cols_; j++) {
      this->matrix_[i][j] *= num;
    }
  }
}

void S21Matrix::MulMatrix(const S21Matrix& other) {
  if (this->cols_ != other.rows_) {
    throw invalid_argument("MulMatrix: Invalid size of matrix.");
  }
  S21Matrix result(this->rows_, other.cols_);
  for (int i = 0; i < this->rows_; i++) {
    for (int j = 0; j < other.cols_; j++) {
      double proz = 0.0;
      for (int k = 0; k < this->cols_; k++) {
        proz += this->matrix_[i][k] * other.matrix_[k][j];
      }
      result.matrix_[i][j] = proz;
    }
  }
  *this = result;
}

S21Matrix S21Matrix::Transpose() const {
  S21Matrix result(this->cols_, this->rows_);
  for (int i = 0; i < this->rows_; i++) {
    for (int j = 0; j < cols_; j++) {
      result.matrix_[j][i] = this->matrix_[i][j];
    }
  }
  return result;
}
S21Matrix S21Matrix::Minor(int i, int j) const {
  S21Matrix minor(this->rows_ - 1, this->cols_ - 1);
  for (int x = 0; x < i; x++) {
    for (int y = 0; y < this->cols_; y++) {
      if (y < j) {
        minor.matrix_[x][y] = this->matrix_[x][y];
      } else if (y > j) {
        minor.matrix_[x][y - 1] = this->matrix_[x][y];
      }
    }
  }
  for (int x = i + 1; x < this->rows_; x++) {
    for (int y = 0; y < this->cols_; y++) {
      if (y < j) {
        minor.matrix_[x - 1][y] = this->matrix_[x][y];
      } else if (y > j) {
        minor.matrix_[x - 1][y - 1] = this->matrix_[x][y];
      }
    }
  }
  return minor;
}
double S21Matrix::Determinant() const {
  if (this->rows_ != this->cols_) {
    throw invalid_argument("Detrminant: No square matrix.");
  }
  if (this->rows_ == 1) {
    return this->matrix_[0][0];
  } else if (this->rows_ == 2) {
    return this->matrix_[0][0] * this->matrix_[1][1] -
           this->matrix_[1][0] * this->matrix_[0][1];
  }
  double result = 0.0;
  for (int i = 0; i < this->rows_; i++) {
    S21Matrix minor = Minor(0, i);
    result += pow(-1, i) * this->matrix_[0][i] * minor.Determinant();
  }
  return result;
}
S21Matrix S21Matrix::CalcComplements() const {
  if (this->rows_ != this->cols_ || this->rows_ == 1) {
    throw invalid_argument("CalcComplement: No square matrix.");
  }
  S21Matrix result(this->rows_, this->cols_);
  for (int i = 0; i < this->rows_; i++) {
    for (int j = 0; j < this->cols_; j++) {
      S21Matrix minor = this->Minor(i, j);
      result.matrix_[i][j] = pow(-1, i + j) * minor.Determinant();
    }
  }
  return result;
}

S21Matrix S21Matrix::InverseMatrix() const {
  double result = this->Determinant();
  if (fabs(result) < MIN_RES) {
    throw invalid_argument("InverseMatrix: Zero Determinant.");
  }
  S21Matrix inverse(this->rows_, this->cols_);
  if (this->rows_ == 1) {
    inverse.matrix_[0][0] = 1 / this->matrix_[0][0];
  } else {
    inverse = this->CalcComplements().Transpose();
    inverse.MulNumber(1.0 / result);
  }
  return inverse;
}
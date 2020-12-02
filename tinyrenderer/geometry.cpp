//
//  geometry.cpp
//  tinyrenderer
//
//  Created by skychx on 2020/11/29.
//
#include <vector>
#include <cassert>
#include <cmath>
#include <iostream>
#include "geometry.h"

vec3 cross(const vec3 &v1, const vec3 &v2) {
    return vec<3>{v1.y*v2.z - v1.z*v2.y, v1.z*v2.x - v1.x*v2.z, v1.x*v2.y - v1.y*v2.x};
}


Matrix::Matrix(int r, int c) : m(std::vector<std::vector<float> >(r, std::vector<float>(c, 0.f))), rows(r), cols(c) { }

// 行数
int Matrix::nrows() {
    return rows;
}

// 列数
int Matrix::ncols() {
    return cols;
}

// 单位矩阵
Matrix Matrix::identity(int dimensions) {
    Matrix E(dimensions, dimensions);
    for (int i = 0; i < dimensions; i++) {
        for (int j = 0; j < dimensions; j++) {
            E[i][j] = (i == j ? 1.f : 0.f);
        }
    }
    return E;
}

std::vector<float>& Matrix::operator[](const int i) {
    assert(i >= 0 && i < rows);
    return m[i];
}

// 矩阵乘法
Matrix Matrix::operator*(const Matrix& a) {
    assert(cols == a.rows);
    Matrix result(rows, a.cols);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < a.cols; j++) {
            result.m[i][j] = 0.f;
            for (int k = 0; k < cols; k++) {
                result.m[i][j] += m[i][k] * a.m[k][j];
            }
        }
    }
    return result;
}

// 矩阵转置
Matrix Matrix::transpose() {
    Matrix result(cols, rows);
    for(int i = 0; i < rows; i++)
        for(int j = 0; j < cols; j++)
            result[j][i] = m[i][j];
    return result;
}

// 逆矩阵
Matrix Matrix::inverse() {
    assert(rows == cols);
    // augmenting the square matrix with the identity matrix of the same dimensions A => [AI]
    Matrix result(rows, cols*2);
    for(int i = 0; i < rows; i++) {
        for(int j = 0; j < cols; j++) {
            result[i][j] = m[i][j];
        }
    }
    
    for(int i = 0; i < rows; i++) {
        result[i][i + cols] = 1;
    }
        
    // first pass
    for (int i = 0; i < rows - 1; i++) {
        // normalize the first row
        for(int j = result.cols - 1; j >= 0; j--)
            result[i][j] /= result[i][i];
        for (int k = i + 1; k < rows; k++) {
            float coeff = result[k][i];
            for (int j = 0; j < result.cols; j++) {
                result[k][j] -= result[i][j]*coeff;
            }
        }
    }
    // normalize the last row
    for(int j = result.cols - 1; j >= rows-1; j--) {
        result[rows-1][j] /= result[rows-1][rows-1];
    }

    // second pass
    for (int i = rows-1; i > 0; i--) {
        for (int k = i-1; k >= 0; k--) {
            float coeff = result[k][i];
            for (int j = 0; j < result.cols; j++) {
                result[k][j] -= result[i][j] * coeff;
            }
        }
    }
    // cut the identity matrix back
    Matrix truncate(rows, cols);
    for(int i = 0; i < rows; i++) {
        for(int j = 0; j < cols; j++) {
            truncate[i][j] = result[i][j + cols];
        }
    }

    return truncate;
}

std::ostream& operator<<(std::ostream& s, Matrix& m) {
    for (int i = 0; i < m.nrows(); i++)  {
        for (int j = 0; j < m.ncols(); j++) {
            s << m[i][j];
            if (j < m.ncols() - 1) s << "\t";
        }
        s << "\n";
    }
    return s;
}

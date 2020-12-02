//
//  geometry.h
//  tinyrenderer
//
//  Created by skychx on 2020/11/15.
//


#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

#include <cmath>
#include <cassert>
#include <iostream>

template<int n> struct vec {
    vec() = default;
    double & operator[](const int i)       { assert(i>=0 && i<n); return data[i]; }
    double   operator[](const int i) const { assert(i>=0 && i<n); return data[i]; }
    double norm2() const { return (*this)*(*this) ; }
    double norm()  const { return std::sqrt(norm2()); } // 欧几里得范数，就是向量长度
    double data[n] = {0};
};

// 向量乘法
template<int n> double operator*(const vec<n>& lhs, const vec<n>& rhs) {
    double ret = 0;
    for (int i=n; i--; ret+=lhs[i]*rhs[i]);
    return ret;
}

// 向量加法
template<int n> vec<n> operator+(const vec<n>& lhs, const vec<n>& rhs) {
    vec<n> ret = lhs;
    for (int i=n; i--; ret[i]+=rhs[i]);
    return ret;
}

// 向量减法
template<int n> vec<n> operator-(const vec<n>& lhs, const vec<n>& rhs) {
    vec<n> ret = lhs;
    for (int i=n; i--; ret[i]-=rhs[i]);
    return ret;
}

// 标量左乘
template<int n> vec<n> operator*(const double& rhs, const vec<n> &lhs) {
    vec<n> ret = lhs;
    for (int i=n; i--; ret[i]*=rhs);
    return ret;
}

// 标量右乘
template<int n> vec<n> operator*(const vec<n>& lhs, const double& rhs) {
    vec<n> ret = lhs;
    for (int i=n; i--; ret[i]*=rhs);
    return ret;
}

// 除以标量
template<int n> vec<n> operator/(const vec<n>& lhs, const double& rhs) {
    vec<n> ret = lhs;
    for (int i=n; i--; ret[i]/=rhs);
    return ret;
}

//
template<int n1,int n2> vec<n1> embed(const vec<n2> &v, double fill=1) {
    vec<n1> ret;
    for (int i=n1; i--; ret[i]=(i<n2?v[i]:fill));
    return ret;
}

// 向量投影
template<int n1,int n2> vec<n1> proj(const vec<n2> &v) {
    vec<n1> ret;
    for (int i=n1; i--; ret[i]=v[i]);
    return ret;
}


template<int n> std::ostream& operator<<(std::ostream& out, const vec<n>& v) {
    for (int i=0; i<n; i++) out << v[i] << " ";
    return out;
}

/////////////////////////////////////////////////////////////////////////////////

template<> struct vec<2> {
    vec() =  default;
    vec(double X, double Y) : x(X), y(Y) {}
    double& operator[](const int i)       { assert(i>=0 && i<2); return i==0 ? x : y; }
    double  operator[](const int i) const { assert(i>=0 && i<2); return i==0 ? x : y; }
    double norm2() const { return (*this)*(*this) ; }
    double norm()  const { return std::sqrt(norm2()); }
    vec & normalize() { *this = (*this)/norm(); return *this; }

    double x{}, y{};
};

/////////////////////////////////////////////////////////////////////////////////

template<> struct vec<3> {
    vec() = default;
    vec(double X, double Y, double Z) : x(X), y(Y), z(Z) {}
    double& operator[](const int i)       { assert(i>=0 && i<3); return i==0 ? x : (1==i ? y : z); }
    double  operator[](const int i) const { assert(i>=0 && i<3); return i==0 ? x : (1==i ? y : z); }
    double norm2() const { return (*this)*(*this) ; }
    double norm()  const { return std::sqrt(norm2()); }
    vec & normalize() { *this = (*this)/norm(); return *this; }

    double x{}, y{}, z{};
};

/////////////////////////////////////////////////////////////////////////////////

typedef vec<2> vec2;
typedef vec<3> vec3;
typedef vec<4> vec4;

// 三维向量叉乘
vec3 cross(const vec3 &v1, const vec3 &v2);

/////////////////////////////////////////////////////////////////////////////////

const int DEFAULT_ALLOC=4;

class Matrix {
    std::vector<std::vector<float> > m;
    int rows, cols;
public:
    Matrix(int r=DEFAULT_ALLOC, int c=DEFAULT_ALLOC);
    inline int nrows();
    inline int ncols();

    static Matrix identity(int dimensions);
    std::vector<float>& operator[](const int i);
    Matrix operator*(const Matrix& a);
    Matrix transpose();
    Matrix inverse();

    friend std::ostream& operator<<(std::ostream& s, Matrix& m);
};

#endif //__GEOMETRY_H__

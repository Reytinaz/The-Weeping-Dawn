#ifndef VECTOR4_HPP
#define VECTOR4_HPP

#include "matrix4.hpp"  // для умножения матрицы на вектор
#include <cmath>

struct Vector4 {
    float x, y, z, w;

    // Конструкторы
    Vector4();
    Vector4(float x, float y, float z, float w = 1.0f);
    Vector4(const Vector3& v, float w = 1.0f);

    // Арифметические операторы (inline для производительности)
    Vector4 operator+(const Vector4& other) const;
    Vector4 operator-(const Vector4& other) const;
    Vector4 operator*(float scalar) const;
    Vector4 operator/(float scalar) const;

    Vector4& operator+=(const Vector4& other);
    Vector4& operator-=(const Vector4& other);
    Vector4& operator*=(float scalar);
    Vector4& operator/=(float scalar);

    // Длина и нормализация (реализация в .cpp)
    float length() const;
    Vector4 normalized() const;

    // Скалярное произведение
    float dot(const Vector4& other) const;

    // Преобразование в Vector3
    Vector3 toVector3() const;

    // Доступ по индексу
    float& operator[](int index);
    const float& operator[](int index) const;
};

// Умножение матрицы на вектор (оставляем inline, так как это просто формула)
inline Vector4 operator*(const Matrix4& m, const Vector4& v) {
    const float* d = m.ptr();
    return Vector4(
        d[0] * v.x + d[4] * v.y + d[8] * v.z + d[12] * v.w,
        d[1] * v.x + d[5] * v.y + d[9] * v.z + d[13] * v.w,
        d[2] * v.x + d[6] * v.y + d[10] * v.z + d[14] * v.w,
        d[3] * v.x + d[7] * v.y + d[11] * v.z + d[15] * v.w
    );
}

#endif
#ifndef MATRIX4_HPP
#define MATRIX4_HPP

#include "vector3.hpp"
#include "vector2.hpp"

class Matrix4 {
public:
    float data[16]; // column-major

    Matrix4();

    static Matrix4 identity();
    static Matrix4 translation(const Vector3& v);
    static Matrix4 rotationX(float angle);
    static Matrix4 rotationY(float angle);
    static Matrix4 rotationZ(float angle);
    static Matrix4 scale(const Vector3& v);
    static Matrix4 perspective(float fov, float aspect, float near, float far);
    static Matrix4 lookAt(const Vector3& eye, const Vector3& target, const Vector3& up);
    static Matrix4 orthographic(float left, float right, float bottom, float top, float near, float far);
    Matrix4 inverse() const;
    Vector3 transformPoint(const Vector3& v);

    Matrix4 operator*(const Matrix4& other) const;
    Vector3 operator*(const Vector3& v) const;

    float* ptr() { return data; }
    const float* ptr() const { return data; }
};

#endif
#include "vector4.hpp"
#include <cmath>

Vector4::Vector4() : x(0), y(0), z(0), w(1) {}

Vector4::Vector4(float x, float y, float z, float w)
    : x(x), y(y), z(z), w(w) {
}

Vector4::Vector4(const Vector3& v, float w)
    : x(v.x), y(v.y), z(v.z), w(w) {
}

Vector4 Vector4::operator+(const Vector4& other) const {
    return Vector4(x + other.x, y + other.y, z + other.z, w + other.w);
}

Vector4 Vector4::operator-(const Vector4& other) const {
    return Vector4(x - other.x, y - other.y, z - other.z, w - other.w);
}

Vector4 Vector4::operator*(float scalar) const {
    return Vector4(x * scalar, y * scalar, z * scalar, w * scalar);
}

Vector4 Vector4::operator/(float scalar) const {
    return Vector4(x / scalar, y / scalar, z / scalar, w / scalar);
}

Vector4& Vector4::operator+=(const Vector4& other) {
    x += other.x; y += other.y; z += other.z; w += other.w;
    return *this;
}

Vector4& Vector4::operator-=(const Vector4& other) {
    x -= other.x; y -= other.y; z -= other.z; w -= other.w;
    return *this;
}

Vector4& Vector4::operator*=(float scalar) {
    x *= scalar; y *= scalar; z *= scalar; w *= scalar;
    return *this;
}

Vector4& Vector4::operator/=(float scalar) {
    x /= scalar; y /= scalar; z /= scalar; w /= scalar;
    return *this;
}

float Vector4::length() const {
    return std::sqrt(x * x + y * y + z * z + w * w);
}

Vector4 Vector4::normalized() const {
    float len = length();
    if (len < 1e-6f) return *this;
    return *this / len;
}

float Vector4::dot(const Vector4& other) const {
    return x * other.x + y * other.y + z * other.z + w * other.w;
}

Vector3 Vector4::toVector3() const {
    return Vector3(x, y, z);
}

float& Vector4::operator[](int index) {
    return (&x)[index];
}

const float& Vector4::operator[](int index) const {
    return (&x)[index];
}
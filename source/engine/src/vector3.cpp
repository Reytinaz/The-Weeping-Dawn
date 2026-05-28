#include "vector3.hpp"

Vector3::Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

Vector3 Vector3::operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
Vector3 Vector3::operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
Vector3 Vector3::operator*(float s) const { return Vector3(x * s, y * s, z * s); }
Vector3 Vector3::operator/(float s) const { return Vector3(x / s, y / s, z / s); }
Vector3 Vector3::operator*=(float s) const { return Vector3(x * s, y * s, z * s); }
Vector3 Vector3::operator/=(float s) const { return Vector3(x / s, y / s, z / s); }
Vector3& Vector3::operator+=(const Vector3& v) { x += v.x; y += v.y; z += v.z; return *this; }
Vector3& Vector3::operator-=(const Vector3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
Vector3 Vector3::operator-() const { return Vector3(-x, -y, -z); }

float Vector3::dot(const Vector3& v) const { return x * v.x + y * v.y + z * v.z; }
Vector3 Vector3::cross(const Vector3& v) const {
    return Vector3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
}
float Vector3::length() const { return std::sqrt(x * x + y * y + z * z); }
Vector3 Vector3::normalized() const { float l = length(); return l > 0 ? *this / l : *this; }
float Vector3::distanceTo(const Vector3& v) const { return (*this - v).length(); }

std::ostream& operator<<(std::ostream& os, const Vector3& v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}
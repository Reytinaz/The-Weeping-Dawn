#ifndef VECTOR3_HPP
#define VECTOR3_HPP

#include "cmath"
#include "iostream"
#include "string"
#include "vector"
#include "cstring"
class Vector3 {
public:
    float x, y, z;

    Vector3(float x = 0, float y = 0, float z = 0);

    Vector3 operator+(const Vector3& v) const;
    Vector3 operator-(const Vector3& v) const;
    Vector3 operator*(float s) const;
    Vector3 operator/(float s) const;
    Vector3 operator*=(float s) const;
    Vector3 operator/=(float s) const;
    Vector3& operator+=(const Vector3& v);
    Vector3& operator-=(const Vector3& v);
    Vector3 operator-() const;

    float dot(const Vector3& v) const;
    Vector3 cross(const Vector3& v) const;
    float length() const;
    Vector3 normalized() const;
    float distanceTo(const Vector3& v) const;

    friend std::ostream& operator<<(std::ostream& os, const Vector3& v);
};

#endif
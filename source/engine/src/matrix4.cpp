#include "matrix4.hpp"

Matrix4::Matrix4() {
    memset(data, 0, 16 * sizeof(float));
}

Matrix4 Matrix4::identity() {
    Matrix4 m;
    m.data[0] = m.data[5] = m.data[10] = m.data[15] = 1.0f;
    return m;
}

Matrix4 Matrix4::translation(const Vector3& v) {
    Matrix4 m = identity();
    m.data[12] = v.x;
    m.data[13] = v.y;
    m.data[14] = v.z;
    return m;
}

Matrix4 Matrix4::rotationX(float angle) {
    Matrix4 m = identity();
    float c = cos(angle), s = sin(angle);
    m.data[5] = c;   // (1,1)
    m.data[6] = s;   // (1,2)
    m.data[9] = -s;  // (2,1)
    m.data[10] = c;   // (2,2)
    return m;
}

Matrix4 Matrix4::rotationY(float angle) {
    Matrix4 m = identity();
    float c = cos(angle), s = sin(angle);
    m.data[0] = c;   // (0,0)
    m.data[2] = -s;  // (0,2)
    m.data[8] = s;   // (2,0)
    m.data[10] = c;   // (2,2)
    return m;
}

Matrix4 Matrix4::rotationZ(float angle) {
    Matrix4 m = identity();
    float c = cos(angle), s = sin(angle);
    m.data[0] = c;   // (0,0)
    m.data[1] = s;   // (0,1)
    m.data[4] = -s;  // (1,0)
    m.data[5] = c;   // (1,1)
    return m;
}

Matrix4 Matrix4::scale(const Vector3& v) {
    Matrix4 m = identity();
    m.data[0] = v.x;
    m.data[5] = v.y;
    m.data[10] = v.z;
    return m;
}

Vector3 Matrix4::transformPoint(const Vector3& v) {
    return Vector3(
        data[0] * v.x + data[4] * v.y + data[8] * v.z + data[12],
        data[1] * v.x + data[5] * v.y + data[9] * v.z + data[13],
        data[2] * v.x + data[6] * v.y + data[10] * v.z + data[14]
    );
}

Matrix4 Matrix4::perspective(float fov, float aspect, float near, float far) {
    Matrix4 m;
    float tanHalf = tan(fov / 2.0f);
    m.data[0] = 1.0f / (aspect * tanHalf);
    m.data[5] = 1.0f / tanHalf;
    m.data[10] = -(far + near) / (far - near);
    m.data[11] = -1.0f;
    m.data[14] = -(2.0f * far * near) / (far - near);
    return m;
}

/*
Matrix4 Matrix4::lookAt(const Vector3& eye, const Vector3& target, const Vector3& up) {
    
    Vector3 forward = (target - eye).normalized();
    Vector3 right = up.cross(forward).normalized();
    Vector3 newUp = forward.cross(right).normalized();

    Matrix4 m;
    m.data[0] = right.x;   m.data[4] = right.y;   m.data[8] = right.z;   m.data[12] = -right.dot(eye);
    m.data[1] = newUp.x;   m.data[5] = newUp.y;   m.data[9] = newUp.z;   m.data[13] = -newUp.dot(eye);
    m.data[2] = forward.x; m.data[6] = forward.y; m.data[10] = forward.z; m.data[14] = -forward.dot(eye);
    m.data[3] = 0;         m.data[7] = 0;         m.data[11] = 0;         m.data[15] = 1.0f;
    return m;
    

    /*
    Vector3 z = (eye - target).normalized();
    Vector3 x = up.cross(z).normalized();
    Vector3 y = z.cross(x);

    Matrix4 m;
    m.data[0] = x.x; m.data[4] = y.x; m.data[8] = z.x; m.data[12] = -x.dot(eye);
    m.data[1] = x.y; m.data[5] = y.y; m.data[9] = z.y; m.data[13] = -y.dot(eye);
    m.data[2] = x.z; m.data[6] = y.z; m.data[10] = z.z; m.data[14] = -z.dot(eye);
    m.data[3] = 0;   m.data[7] = 0;   m.data[11] = 0;    m.data[15] = 1;
    return m;
    
}
*/

Matrix4 Matrix4::lookAt(const Vector3& eye, const Vector3& target, const Vector3& up) {
    Vector3 z = (eye - target).normalized(); // направление взгляда от target к eye
    Vector3 x = up.cross(z).normalized();    // правое направление
    Vector3 y = z.cross(x).normalized();     // скорректированный up

    Matrix4 m;
    m.data[0] = x.x; m.data[4] = y.x; m.data[8] = z.x; m.data[12] = -x.dot(eye);
    m.data[1] = x.y; m.data[5] = y.y; m.data[9] = z.y; m.data[13] = -y.dot(eye);
    m.data[2] = x.z; m.data[6] = y.z; m.data[10] = z.z; m.data[14] = -z.dot(eye);
    m.data[3] = 0;   m.data[7] = 0;   m.data[11] = 0;   m.data[15] = 1.0f;
    return m;
}


Matrix4 Matrix4::orthographic(float left, float right, float bottom, float top, float near, float far) {
    Matrix4 m;
    float rl = right - left;
    float tb = top - bottom;
    float fn = far - near;

    m.data[0] = 2.0f / rl;
    m.data[5] = 2.0f / tb;
    m.data[10] = -2.0f / fn;
    m.data[12] = -(right + left) / rl;
    m.data[13] = -(top + bottom) / tb;
    m.data[14] = -(far + near) / fn;
    m.data[15] = 1.0f;
    return m;
}

Matrix4 Matrix4::inverse() const {
    Matrix4 inv;
    float det;
    int i;

    float m[16];
    for (i = 0; i < 16; ++i) m[i] = data[i];

    inv.data[0] = m[5] * m[10] * m[15] -
        m[5] * m[11] * m[14] -
        m[9] * m[6] * m[15] +
        m[9] * m[7] * m[14] +
        m[13] * m[6] * m[11] -
        m[13] * m[7] * m[10];

    inv.data[4] = -m[4] * m[10] * m[15] +
        m[4] * m[11] * m[14] +
        m[8] * m[6] * m[15] -
        m[8] * m[7] * m[14] -
        m[12] * m[6] * m[11] +
        m[12] * m[7] * m[10];

    inv.data[8] = m[4] * m[9] * m[15] -
        m[4] * m[11] * m[13] -
        m[8] * m[5] * m[15] +
        m[8] * m[7] * m[13] +
        m[12] * m[5] * m[11] -
        m[12] * m[7] * m[9];

    inv.data[12] = -m[4] * m[9] * m[14] +
        m[4] * m[10] * m[13] +
        m[8] * m[5] * m[14] -
        m[8] * m[6] * m[13] -
        m[12] * m[5] * m[10] +
        m[12] * m[6] * m[9];

    inv.data[1] = -m[1] * m[10] * m[15] +
        m[1] * m[11] * m[14] +
        m[9] * m[2] * m[15] -
        m[9] * m[3] * m[14] -
        m[13] * m[2] * m[11] +
        m[13] * m[3] * m[10];

    inv.data[5] = m[0] * m[10] * m[15] -
        m[0] * m[11] * m[14] -
        m[8] * m[2] * m[15] +
        m[8] * m[3] * m[14] +
        m[12] * m[2] * m[11] -
        m[12] * m[3] * m[10];

    inv.data[9] = -m[0] * m[9] * m[15] +
        m[0] * m[11] * m[13] +
        m[8] * m[1] * m[15] -
        m[8] * m[3] * m[13] -
        m[12] * m[1] * m[11] +
        m[12] * m[3] * m[9];

    inv.data[13] = m[0] * m[9] * m[14] -
        m[0] * m[10] * m[13] -
        m[8] * m[1] * m[14] +
        m[8] * m[2] * m[13] +
        m[12] * m[1] * m[10] -
        m[12] * m[2] * m[9];

    inv.data[2] = m[1] * m[6] * m[15] -
        m[1] * m[7] * m[14] -
        m[5] * m[2] * m[15] +
        m[5] * m[3] * m[14] +
        m[13] * m[2] * m[7] -
        m[13] * m[3] * m[6];

    inv.data[6] = -m[0] * m[6] * m[15] +
        m[0] * m[7] * m[14] +
        m[4] * m[2] * m[15] -
        m[4] * m[3] * m[14] -
        m[12] * m[2] * m[7] +
        m[12] * m[3] * m[6];

    inv.data[10] = m[0] * m[5] * m[15] -
        m[0] * m[7] * m[13] -
        m[4] * m[1] * m[15] +
        m[4] * m[3] * m[13] +
        m[12] * m[1] * m[7] -
        m[12] * m[3] * m[5];

    inv.data[14] = -m[0] * m[5] * m[14] +
        m[0] * m[6] * m[13] +
        m[4] * m[1] * m[14] -
        m[4] * m[2] * m[13] -
        m[12] * m[1] * m[6] +
        m[12] * m[2] * m[5];

    inv.data[3] = -m[1] * m[6] * m[11] +
        m[1] * m[7] * m[10] +
        m[5] * m[2] * m[11] -
        m[5] * m[3] * m[10] -
        m[9] * m[2] * m[7] +
        m[9] * m[3] * m[6];

    inv.data[7] = m[0] * m[6] * m[11] -
        m[0] * m[7] * m[10] -
        m[4] * m[2] * m[11] +
        m[4] * m[3] * m[10] +
        m[8] * m[2] * m[7] -
        m[8] * m[3] * m[6];

    inv.data[11] = -m[0] * m[5] * m[11] +
        m[0] * m[7] * m[9] +
        m[4] * m[1] * m[11] -
        m[4] * m[3] * m[9] -
        m[8] * m[1] * m[7] +
        m[8] * m[3] * m[5];

    inv.data[15] = m[0] * m[5] * m[10] -
        m[0] * m[6] * m[9] -
        m[4] * m[1] * m[10] +
        m[4] * m[2] * m[9] +
        m[8] * m[1] * m[6] -
        m[8] * m[2] * m[5];

    det = m[0] * inv.data[0] + m[1] * inv.data[4] + m[2] * inv.data[8] + m[3] * inv.data[12];
    if (det == 0) return Matrix4::identity();

    det = 1.0f / det;
    for (i = 0; i < 16; ++i) inv.data[i] *= det;

    return inv;
}

Matrix4 Matrix4::operator*(const Matrix4& other) const {
    Matrix4 res;
    for (int i = 0; i < 4; ++i) {           // строка i
        for (int j = 0; j < 4; ++j) {       // столбец j
            float sum = 0;
            for (int k = 0; k < 4; ++k) {
                // data[i][k] * other[k][j] в column-major: data[i + 4*k] * other[k + 4*j]
                sum += data[i + 4 * k] * other.data[k + 4 * j];
            }
            res.data[i + 4 * j] = sum;
        }
    }
    return res;
}

Vector3 Matrix4::operator*(const Vector3& v) const {
    // Умножение матрицы на вектор (w считается равным 1)
    return Vector3(
        data[0] * v.x + data[4] * v.y + data[8] * v.z + data[12],
        data[1] * v.x + data[5] * v.y + data[9] * v.z + data[13],
        data[2] * v.x + data[6] * v.y + data[10] * v.z + data[14]
    );
}
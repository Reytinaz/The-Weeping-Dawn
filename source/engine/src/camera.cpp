#include "camera.hpp"

Camera::Camera(float aspect)
    : position(0, 2, 10), rotation(0, 0, 0),
    fov(45.0f * 3.14159f / 180.0f), nearPlane(0.1f), farPlane(1000.0f),
    aspectRatio(aspect) {
}

Vector3 Camera::forward() const {
    Vector3 result = Vector3(
        sin(rotation.y) * cos(rotation.x),
       -sin(rotation.x),
        cos(rotation.y) * cos(rotation.x)
    ).normalized();
    return -result;
}

Vector3 Camera::right() const {
    return -Vector3(cos(rotation.y), 0, -sin(rotation.y)).normalized();
}

Vector3 Camera::up() const {
    return Vector3(0, 1, 0);
}

Matrix4 Camera::getViewMatrix() const {
    //Vector3 target = position + forward();
    //return Matrix4::lookAt(position, target, up());

    //Matrix4 rotation1 = Matrix4::rotationY(-rotation.y) * Matrix4::rotationX(-rotation.x);
    //Matrix4 translation = Matrix4::translation(-position);
    //return rotation1 * translation;
    float yaw = rotation.y;
    float pitch = rotation.x;
    Vector3 X(cos(yaw), 0, -sin(yaw));
    Vector3 Y(-sin(pitch) * sin(yaw), cos(pitch), -sin(pitch) * cos(yaw));
    Vector3 Z(cos(pitch) * sin(yaw), sin(pitch), cos(pitch) * cos(yaw));
    Matrix4 view;
    view.data[0] = X.x; view.data[4] = X.y; view.data[8] = X.z; view.data[12] = -X.dot(position);
    view.data[1] = Y.x; view.data[5] = Y.y; view.data[9] = Y.z; view.data[13] = -Y.dot(position);
    view.data[2] = Z.x; view.data[6] = Z.y; view.data[10] = Z.z; view.data[14] = -Z.dot(position);
    view.data[3] = 0;   view.data[7] = 0;   view.data[11] = 0;   view.data[15] = 1;
    return view;
}

Matrix4 Camera::getProjectionMatrix() const {
    return Matrix4::perspective(fov, aspectRatio, nearPlane, farPlane);
}
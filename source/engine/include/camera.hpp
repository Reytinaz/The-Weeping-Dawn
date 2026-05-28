#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "vector3.hpp"
#include "matrix4.hpp"

enum class CameraType {
    FIRST_PERSON,
    THIRD_PERSON,
    FREECAM,
};

class Camera {
public:
    Vector3 position;
    Vector3 rotation; 
    Vector3 target;
    float fov;
    float nearPlane;
    float farPlane;
    float aspectRatio;

    CameraType cameraType = CameraType::FIRST_PERSON;

    Camera(float aspect = 16.0f / 9.0f);

    Vector3 forward() const;
    Vector3 right() const;
    Vector3 up() const;
    void setTarget(const Vector3& t) { target = t; }
    Vector3 getTarget() const { return target; }

    Vector3 getForward() const {
        return (target - position).normalized();
    }

    Matrix4 getViewMatrix() const;
    Matrix4 getProjectionMatrix() const;
    Matrix4 getProjectionMatrixForRange(float near, float far) const {
        return Matrix4::perspective(fov, aspectRatio, near, far);
    }
    void updateVectorsFromView() {
        Matrix4 viewMatrix = getViewMatrix();
        Vector3 forward = Vector3(-viewMatrix.data[2], -viewMatrix.data[6], -viewMatrix.data[10]).normalized();
        float distance = (target - position).length();
        target = position + forward * distance;
    }
};

#endif
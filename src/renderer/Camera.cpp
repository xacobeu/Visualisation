#include "Camera.hpp"
#include "util/Pi.hpp"

void Camera::update(double mouseX, double mouseY) {
    double dx = mouseX - lastMouseX;
    double dy = mouseY - lastMouseY;

    lastMouseX = mouseX;
    lastMouseY = mouseY;

    rotate(float(dx * 0.3f), float(dy * 0.3f));
}

Matrix4 Camera::getViewMatrix() const {
    return Matrix4::lookAt(
        getPosition(),              // camera position
        {0, 0, 0},                  // look at origin
        {0, 1, 0}                   // up direction
    );
}

Matrix4 Camera::getProjectionMatrix(float aspect, float fov) const {
    return Matrix4::perspective(fov, aspect, 0.1f, 100.0f);
}

Vector3 Camera::getPosition() const {
    float yawRad = yaw * PI / 180.0f;
    float pitchRad = pitch * PI / 180.0f;
    
    return {
        distance * cos(pitchRad) * cos(yawRad),
        distance * sin(pitchRad),
        distance * cos(pitchRad) * sin(yawRad)
    };
}

void Camera::rotate(float deltaYaw, float deltaPitch) {
    yaw += deltaYaw;
    pitch += deltaPitch;

    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
}

void Camera::zoom(float yoff) {
    distance -= yoff * 0.3f;
    if (distance < 1.5f) distance = 1.5f;
    if (distance > 10.0f) distance = 10.0f;
}


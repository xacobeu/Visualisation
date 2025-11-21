#pragma once

#include "util/MathUtils.hpp"
#include "util/Matrix4.hpp"

class Camera {
public:
    float yaw = 0.0f;
    float pitch = 0.0f;
    float distance = 3.0f;

    bool dragging = false;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;

    void update(double mouseX, double mouseY);
    void zoom(float yoff);

    Matrix4 getViewMatrix() const;
    Matrix4 getProjectionMatrix(float aspect, float fov = 45.0f) const;
    Vector3 getPosition() const;

private:
    void rotate(float deltaYaw, float deltaPitch);
};
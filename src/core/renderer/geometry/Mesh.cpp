#include "Mesh.hpp"

Mesh& Mesh::translate(const Vector3& t) {
    for (auto& v : data.vertices)
        v.position = v.position + t;
    return *this;
}

Mesh& Mesh::scale(const Vector3& s) {
    for (auto& v : data.vertices) {
        v.position = v.position * s;
        v.normal = normalize(v.normal / s);
    }
    return *this;
}

Mesh& Mesh::scale(float s) {
    scale({s, s, s});
    return *this;
}

Mesh& Mesh::rotateX(float angleDeg) {
    float a = angleDeg * PI / 180.0f;
    float c = std::cos(a);
    float s = std::sin(a);
    for (auto& v : data.vertices) {
        auto rotate = [&](float& y, float& z) {
            float ny = y * c - z * s;
            float nz = y * s + z * c;
            y = ny; z = nz;
        };
        rotate(v.position.y, v.position.z);
        rotate(v.normal.y,   v.normal.z);
    }
    return *this;
}

Mesh& Mesh::rotateY(float angleDeg) {
    float a = angleDeg * PI / 180.0f;
    float c = std::cos(a);
    float s = std::sin(a);
    for (auto& v : data.vertices) {
        auto rotate = [&](float& x, float& z) {
            float nx =  x * c + z * s;
            float nz = -x * s + z * c;
            x = nx; z = nz;
        };
        rotate(v.position.x, v.position.z);
        rotate(v.normal.x,   v.normal.z);
    }
    return *this;
}

Mesh& Mesh::rotateZ(float angleDeg) {
    float a = angleDeg * PI / 180.0f;
    float c = std::cos(a);
    float s = std::sin(a);
    for (auto& v : data.vertices) {
        auto rotate = [&](float& x, float& y) {
            float nx = x * c - y * s;
            float ny = x * s + y * c;
            x = nx; y = ny;
        };
        rotate(v.position.x, v.position.y);
        rotate(v.normal.x,   v.normal.y);
    }
    return *this;
}

#pragma once
#include <cmath>
#include "MathUtils.hpp"

struct Matrix4 {
    float m[16]{}; // column-major

    static Matrix4 identity() {
        Matrix4 r{};
        r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
        return r;
    }

    const float* data() const { return m; }

    static Matrix4 perspective(float fovDeg, float aspect, float nearZ, float farZ) {
        Matrix4 r{};
        float f = 1.0f / std::tan((fovDeg * 0.5f) * 3.14159265f / 180.0f);

        r.m[0]  = f / aspect;
        r.m[5]  = f;
        r.m[10] = (farZ + nearZ) / (nearZ - farZ);
        r.m[11] = -1.0f;
        r.m[14] = (2.0f * farZ * nearZ) / (nearZ - farZ);
        return r;
    }

    static Matrix4 frustrum(float left, float right, float bottom, float top, float nearZ, float farZ) {
        Matrix4 r{};
        r.m[0]  = (2.0f * nearZ) / (right - left);
        r.m[5]  = (2.0f * nearZ) / (top - bottom);
        r.m[8]  = (right + left) / (right - left);
        r.m[9]  = (top + bottom) / (top - bottom);
        r.m[10] = -(farZ + nearZ) / (farZ - nearZ);
        r.m[11] = -1.0f;
        r.m[14] = -(2.0f * farZ * nearZ) / (farZ - nearZ);
        return r;
    }

    static Vector3 normalize(const Vector3& v) {
        float l = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z);
        return { v.x/l, v.y/l, v.z/l };
    }

    static Vector3 cross(const Vector3& a, const Vector3& b) {
        return {
            a.y*b.z - a.z*b.y,
            a.z*b.x - a.x*b.z,
            a.x*b.y - a.y*b.x
        };
    }

    static Matrix4 lookAt(const Vector3& eye, const Vector3& target, const Vector3& up) {
        Vector3 f = normalize({ target.x - eye.x, target.y - eye.y, target.z - eye.z });
        Vector3 s = normalize(cross(f, up));
        Vector3 u = cross(s, f);

        Matrix4 r = identity();

        r.m[0] = s.x; r.m[4] = s.y; r.m[8]  = s.z;
        r.m[1] = u.x; r.m[5] = u.y; r.m[9]  = u.z;
        r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;

        r.m[12] = -(s.x*eye.x + s.y*eye.y + s.z*eye.z);
        r.m[13] = -(u.x*eye.x + u.y*eye.y + u.z*eye.z);
        r.m[14] =  (f.x*eye.x + f.y*eye.y + f.z*eye.z);

        return r;
    }
};
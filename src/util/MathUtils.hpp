#pragma once

#include <array>
#include <cmath>
#include <cstddef>

template<std::size_t N, typename T = float>
struct Vec {
    std::array<T, N> v{};

    T& operator[](std::size_t i)             { return v[i]; }
    const T& operator[](std::size_t i) const { return v[i]; }

    Vec operator+(const Vec& rhs) const {
        Vec out;
        for (std::size_t i = 0; i < N; ++i) out[i] = v[i] + rhs[i];
        return out;
    }

    Vec operator-(const Vec& rhs) const {
        Vec out;
        for (std::size_t i = 0; i < N; ++i) out[i] = v[i] - rhs[i];
        return out;
    }

    Vec operator*(T s) const {
        Vec out;
        for (std::size_t i = 0; i < N; ++i) out[i] = v[i] * s;
        return out;
    }

    T dot(const Vec& rhs) const {
        T sum = 0;
        for (std::size_t i = 0; i < N; ++i) sum += v[i] * rhs[i];
        return sum;
    }
};

template<size_t N, typename T>
Vec<N,T> normalize(const Vec<N,T>& v) {
    T len2 = v.dot(v);
    if (len2 == 0) return v;
    T invLen = T(1) / std::sqrt(len2);

    Vec<N,T> out;
    for (size_t i = 0; i < N; i++)
        out[i] = v[i] * invLen;
    return out;
}

template<typename T>
struct Vec<2, T> {
    union {
        struct { T x, y; };
        std::array<T, 2> v;
    };

    T& operator[](std::size_t i)             { return v[i]; }
    const T& operator[](std::size_t i) const { return v[i]; }

    Vec operator+(const Vec& r) const { return {x + r.x, y + r.y}; }
    Vec operator-(const Vec& r) const { return {x - r.x, y - r.y}; }
    Vec operator*(T s) const          { return {x * s, y * s}; }

    T dot(const Vec& r) const         { return x*r.x + y*r.y; }
};

template<typename T>
struct Vec<3, T> {
    union {
        struct { T x, y, z; };
        std::array<T, 3> v;
    };

    // Direction constants
    static const Vec up()       { return { 0,  1,  0}; }
    static const Vec down()     { return { 0, -1,  0}; }
    static const Vec left()     { return {-1,  0,  0}; }
    static const Vec right()    { return { 1,  0,  0}; }
    static const Vec forward()  { return { 0,  0,  1}; }
    static const Vec back()     { return { 0,  0, -1}; }

    T& operator[](std::size_t i)             { return v[i]; }
    const T& operator[](std::size_t i) const { return v[i]; }

    Vec operator+(const Vec& r) const { return {x + r.x, y + r.y, z + r.z}; }
    Vec operator-(const Vec& r) const { return {x - r.x, y - r.y, z - r.z}; }
    Vec operator*(T s) const          { return {x * s, y * s, z * s}; }

    T dot(const Vec& r) const         { return x*r.x + y*r.y + z*r.z; }
};

template<typename T>
Vec<3,T> cross(const Vec<3,T>& a, const Vec<3,T>& b) {
    return {
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0]
    };
}

template<typename T>
struct Vec<4, T> {
    union {
        struct { T x, y, z, w; };
        std::array<T, 4> v;
    };

    T& operator[](std::size_t i)             { return v[i]; }
    const T& operator[](std::size_t i) const { return v[i]; }

    Vec operator+(const Vec& r) const { return {x+r.x, y+r.y, z+r.z, w+r.w}; }
    Vec operator-(const Vec& r) const { return {x-r.x, y-r.y, z-r.z, w-r.w}; }
    Vec operator*(T s) const          { return {x*s, y*s, z*s, w*s}; }

    T dot(const Vec& r) const         {
        return x*r.x + y*r.y + z*r.z + w*r.w;
    }
};

using Vector2 = Vec<2,float>;
using Vector3 = Vec<3,float>;
using Vector4 = Vec<4,float>;
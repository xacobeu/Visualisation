#pragma once

#include <array>
#include <glad/glad.h>
#include "util/Vector.hpp"

struct Vertex {
    Vector3 position;
    Vector3 normal;
    Vector2 uv;
    Vector3 color;

    struct Attribute {
        GLuint index;
        GLint  size;
        GLenum type;
        std::size_t offset;
    };

    static const std::array<Vertex::Attribute, 3> attributes;
};

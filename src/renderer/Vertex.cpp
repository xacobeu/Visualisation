#include "Vertex.hpp"
#include <cstddef>

const std::array<Vertex::Attribute, 3> Vertex::attributes = {
    Vertex::Attribute{0, 3, GL_FLOAT, offsetof(Vertex, position)},
    Vertex::Attribute{1, 3, GL_FLOAT, offsetof(Vertex, normal)},
    Vertex::Attribute{2, 2, GL_FLOAT, offsetof(Vertex, uv)},
};

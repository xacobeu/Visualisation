#pragma once

#include <vector>
#include "util/MathUtils.hpp"
#include "util/Pi.hpp"
#include "renderer/GL/Vertex.hpp"

class Mesh {
public:

    struct MeshData {
        std::vector<Vertex> vertices;
        std::vector<int> triangles;
    };

    Mesh() = default;
    explicit Mesh(MeshData data) : data(std::move(data)) {}
    const MeshData& getData() const { return data; }

    Mesh& translate(const Vector3& t);

    Mesh& scale(const Vector3& s);
    Mesh& scale(float s);

    Mesh& rotateX(float angleDeg);
    Mesh& rotateY(float angleDeg);
    Mesh& rotateZ(float angleDeg);

private:
    MeshData data;
};

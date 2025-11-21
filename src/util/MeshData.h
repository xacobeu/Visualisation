#pragma once

#include <vector>
#include "util/MathUtils.hpp"

struct MeshData {
    std::vector<Vector3> vertices;
    std::vector<Vector3> normals;
    std::vector<Vector2> uvs;
    std::vector<int> triangles;
};
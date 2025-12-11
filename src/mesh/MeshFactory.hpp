#pragma once

#include "Mesh.hpp"

class MeshFactory {
public:
    static Mesh::MeshData createCubeMesh();
    static Mesh::MeshData createSphereMesh(const int resolution, const Vector3& color);
    static Mesh cube() { return Mesh(createCubeMesh()); }
    static Mesh sphere(int resolution, const Vector3& color) { return Mesh(createSphereMesh(resolution, color)); }
};

#include "MeshFactory.hpp"
#include "util/Pi.hpp"
#include "renderer/GL/Vertex.hpp"

Mesh::MeshData MeshFactory::createCubeMesh() {
    Mesh::MeshData mesh;

    static constexpr Vector3 positions[6][4] = {
        { {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1} }, // Front
        { {-1,-1,-1}, {-1, 1,-1}, { 1, 1,-1}, { 1,-1,-1} }, // Back
        { {-1, 1,-1}, {-1, 1, 1}, { 1, 1, 1}, { 1, 1,-1} }, // Top
        { {-1,-1,-1}, { 1,-1,-1}, { 1,-1, 1}, {-1,-1, 1} }, // Bottom
        { { 1,-1,-1}, { 1, 1,-1}, { 1, 1, 1}, { 1,-1, 1} }, // Right
        { {-1,-1,-1}, {-1,-1, 1}, {-1, 1, 1}, {-1, 1,-1} }  // Left
    };

    static constexpr Vector3 normals[6] = {
        {0,0,1},  {0,0,-1}, {0,1,0},
        {0,-1,0}, {1,0,0},  {-1,0,0}
    };

    static constexpr Vector2 uvs[4] = {
        {0,0}, {1,0}, {1,1}, {0,1}
    };

    static constexpr Vector3 colors[6] = {
        {  1, 0.2, 0.2},
        {0.2,   1, 0.2},
        {0.2, 0.2,   1},
        {  1,   1, 0.2},
        {  1, 0.2,   1},
        {0.2,   1,   1}
    };

    // Build vertices
    for (int face = 0; face < 6; face++) {
        for (int i = 0; i < 4; i++) {
            mesh.vertices.push_back(Vertex{
                positions[face][i],
                normals[face],
                uvs[i],
                colors[face]
            });
        }
    }

    // Triangles
    for (int face = 0; face < 6; face++) {
        int base = face * 4;
        mesh.triangles.insert(mesh.triangles.end(), {
            base, base+1, base+2,
            base, base+2, base+3
        });
    }

    return mesh;
}

Mesh::MeshData MeshFactory::createSphereMesh(const int resolution, const Vector3& color) {

    Mesh::MeshData mesh;

    // Generate UV sphere
    for (int lat = 0; lat <= resolution; lat++) {
        for (int lon = 0; lon <= resolution; lon++) {
            float theta = float(lat) / resolution * PI;
            float phi = float(lon) / resolution * TWO_PI;

            // Spherical coordinates to cartesian
            Vector3 pointOnSphere = {
                sin(theta) * cos(phi),
                cos(theta),
                sin(theta) * sin(phi)
            };

            // UV coordinates
            Vector2 uv = {
                1.0f - float(lon) / resolution,
                float(lat) / resolution
            };

            mesh.vertices.push_back(Vertex{
                pointOnSphere,
                normalize(pointOnSphere),
                uv,
                color
            });
        }
    }

    // Generate triangles
    for (int lat = 0; lat < resolution; lat++) {
        for (int lon = 0; lon < resolution; lon++) {
            int current = lat * (resolution + 1) + lon;
            int next = current + resolution + 1;

            // Two triangles per quad
            mesh.triangles.push_back(current);
            mesh.triangles.push_back(next);
            mesh.triangles.push_back(current + 1);

            mesh.triangles.push_back(current + 1);
            mesh.triangles.push_back(next);
            mesh.triangles.push_back(next + 1);
        }
    }

    return mesh;
}

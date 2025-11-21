#include "SphereGenerator.hpp"
#include "util/Pi.hpp"
#include <cmath>
#include <functional>

MeshData SphereGenerator::generateSphere(int resolution, float elevationScale, 
                                         std::function<float(Vector2)> heightSampler) {
    MeshData sphere;
    
    // Generate UV sphere instead of cube-to-sphere to avoid seams
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
            
            float heightOffset = 0.0f;
            if (heightSampler && elevationScale > 0.0f) {
                heightOffset = heightSampler(uv) * elevationScale;
            }
            
            Vector3 offset = pointOnSphere * (1.0f + heightOffset);
            
            sphere.vertices.push_back(offset);
            sphere.uvs.push_back(uv);
            sphere.normals.push_back(normalize(offset));
        }
    }
    
    // Generate triangles
    for (int lat = 0; lat < resolution; lat++) {
        for (int lon = 0; lon < resolution; lon++) {
            int current = lat * (resolution + 1) + lon;
            int next = current + resolution + 1;
            
            // Two triangles per quad
            sphere.triangles.push_back(current);
            sphere.triangles.push_back(next);
            sphere.triangles.push_back(current + 1);
            
            sphere.triangles.push_back(current + 1);
            sphere.triangles.push_back(next);
            sphere.triangles.push_back(next + 1);
        }
    }
    
    return sphere;
}

std::vector<MeshData> SphereGenerator::generateFaces(int resolution) {
    std::vector<MeshData> faces(6);
    std::array<Vector3, 6> normals = {
        Vector3::up(),
        Vector3::down(),
        Vector3::left(),
        Vector3::right(),
        Vector3::forward(),
        Vector3::back()
    };

    for (int i = 0; i < 6; ++i) {
        faces[i] = createFace(normals[i], resolution);
    }

    return faces;
}

MeshData SphereGenerator::createFace(Vector3 normal, int resolution) {
    Vector3 axisA = { normal.y, normal.z, normal.x };
    Vector3 axisB = cross(normal, axisA);

    MeshData face;
    face.vertices.resize(resolution * resolution);
    face.normals.resize(resolution * resolution);
    face.triangles.resize((resolution - 1) * (resolution - 1) * 6);
    int triIndex = 0;

    for (int y = 0; y < resolution; y++) {
        for (int x = 0; x < resolution; x++) {

            int vertexIndex = x + y * resolution;

            Vector2 t {
                float(x) / (resolution - 1),
                float(y) / (resolution - 1)
            };

            // Cube face point
            Vector3 point =
                normal +
                axisA * (2 * t.x - 1) +
                axisB * (2 * t.y - 1);

            face.vertices[vertexIndex] = point;
            face.normals[vertexIndex] = normal;

            if (x != resolution - 1 && y != resolution - 1) {
                face.triangles[triIndex++] = vertexIndex;
                face.triangles[triIndex++] = vertexIndex + resolution + 1;
                face.triangles[triIndex++] = vertexIndex + resolution;

                face.triangles[triIndex++] = vertexIndex;
                face.triangles[triIndex++] = vertexIndex + 1;
                face.triangles[triIndex++] = vertexIndex + resolution + 1;
            }
        }
    }

    return face;
}

Vector3 SphereGenerator::pointOnCubeToPointOnSphere(const Vector3& p) {
    float x2 = p.x * p.x;
    float y2 = p.y * p.y;
    float z2 = p.z * p.z;

    Vector3 result = {
        p.x * std::sqrt(1.0f - (y2 / 2.0f) - (z2 / 2.0f) + (y2 * z2) / 3.0f),
        p.y * std::sqrt(1.0f - (x2 / 2.0f) - (z2 / 2.0f) + (x2 * z2) / 3.0f),
        p.z * std::sqrt(1.0f - (x2 / 2.0f) - (y2 / 2.0f) + (x2 * y2) / 3.0f)
    };

    return result;
}

Vector2 SphereGenerator::pointToUV(const Vector3& pointOnUnitSphere) {
    // Fix orientation: flip U horizontally and V vertically
    float u = 1.0f - (atan2(pointOnUnitSphere.z, pointOnUnitSphere.x) / TWO_PI + 0.5f);
    float v = 1.0f - (pointOnUnitSphere.y * 0.5f + 0.5f);
    
    // Ensure u is in [0, 1] range
    while (u < 0.0f) u += 1.0f;
    while (u > 1.0f) u -= 1.0f;
    
    return {u, v};
}

Vector3 SphereGenerator::UVtoPoint(const Vector2& uv) {
    float longitude = (uv.x - 0.5f) * TWO_PI;
    float latitude  = (uv.y - 0.5f) * PI;

    float y = std::sin(latitude);
    float r = std::cos(latitude);

    float x = std::cos(longitude) * r;
    float z = std::sin(longitude) * r;

    return { x, y, z };
}
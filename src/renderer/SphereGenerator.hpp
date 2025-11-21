#pragma once

#include "util/MathUtils.hpp"
#include "util/MeshData.h"
#include <vector>
#include <functional>

class SphereGenerator {
public:

    static MeshData createFace(Vector3 normal, int resolution);
    static std::vector<MeshData> generateFaces(int resolution);
    static MeshData generateSphere(
        int resolution, 
        float elevationScale = 0.0f, 
        std::function<float(Vector2)> heightSampler = nullptr
    );
    
private:

    static Vector3 pointOnCubeToPointOnSphere(const Vector3& p);
    static Vector2 pointToUV(const Vector3& pointOnUnitSphere);
    static Vector3 UVtoPoint(const Vector2& uv);
};
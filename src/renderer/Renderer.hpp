#pragma once

#include <glad/glad.h>

#include "util/MathUtils.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "util/Pi.hpp"

#include "mesh/MeshFactory.hpp"
#include "mesh/Mesh.hpp"
#include "GL/GLObject.hpp"

class Renderer {
public:
    Renderer() = default;
    ~Renderer() = default;

    bool init();
    void render(int width, int height);

    Camera camera;
    Mesh sphere;

private:

    static constexpr const char* VERT_PATH = "shaders/vertex.glsl";
    static constexpr const char* FRAG_PATH = "shaders/fragment.glsl";

    // Shader
    Shader shader;

    // Globe mesh buffers
    GL::VertexArray VAO;
    GL::VertexBuffer VBO;
    GL::ElementBuffer EBO;
    int indexCount = 0;

    void setupBuffers(const Mesh::MeshData& mesh);  
};

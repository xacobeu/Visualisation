#include "Renderer.hpp"
#include "util/Matrix4.hpp"

#include <vector>
#include <iostream>
#include <cmath>
#include <filesystem>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

bool Renderer::init() {

    setupBuffers(
        MeshFactory::sphere(512, Vector3{0.0f, 0.0f, 0.0f}).getData()
    );

    if (!shader.compileFromFiles(VERT_PATH, FRAG_PATH)) {
        std::cerr << "Failed to compile shaders.\n";
        return false;
    }
    
    return true;
}

void Renderer::render(int width, int height) {

    glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Get camera matrices
    Matrix4 view = camera.getViewMatrix();
    Matrix4 proj = camera.getProjectionMatrix(float(width) / float(height), 45.0f);

    // Shader
    shader.bind();
    shader.setMat4("uView", view.data());
    shader.setMat4("uProj", proj.data());    

    // Draw.
    VAO.bind();
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
    VAO.unbind();
    
    Shader::unbind();
}

// === GPU stuff ===

void Renderer::setupBuffers(const Mesh::MeshData& mesh) {

    VAO.create();
    VBO.create();
    EBO.create();
    
    indexCount = static_cast<int>(mesh.triangles.size());
    
    VAO.bind();
    
    {
        VBO.bind();
        VBO.data(mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), GL_STATIC_DRAW);

        EBO.bind();
        EBO.data(mesh.triangles.size() * sizeof(int), mesh.triangles.data(), GL_STATIC_DRAW);

        VAO.setLayout<Vertex>();
    }

    VAO.unbind();

}

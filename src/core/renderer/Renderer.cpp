#include "Renderer.hpp"
#include "util/Matrix4.hpp"

#include <vector>
#include <iostream>
#include <cmath>
#include <filesystem>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void Renderer::begin(int width, int height) {
    glViewport(0, 0, width, height);
    glClearColor(0.1f, 0.4f, 0.7f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
}

void Renderer::submit(const std::shared_ptr<GL::VertexArray>& vao,
                const std::shared_ptr<int> indexCount,
                const std::shared_ptr<GL::Shader>& shader,
                const Matrix4& transform) {

    shader->bind();
    shader->setMat4("u_Transform", transform.data());

    vao->bind();
    glDrawElements(GL_TRIANGLES, *indexCount, GL_UNSIGNED_INT, 0);
    vao->unbind();
    shader->unbind();
}

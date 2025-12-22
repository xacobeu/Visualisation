#pragma once

#include <memory>
#include "Layer.hpp"
#include "Application.hpp"
#include "renderer/GL/GLObject.hpp"
#include "renderer/GL/Shader.hpp"
#include "renderer/geometry/MeshFactory.hpp"
#include "renderer/Renderer.hpp"
#include "renderer/Camera.hpp"

class GlobeLayer : public Layer {

public:

    void setCallbacks();

    void onAttach() override {
        vao->create();
        vbo->create();
        ebo->create();

        // Get mesh data
        const auto& meshData = cube.getData();
        *indexCount = static_cast<int>(meshData.triangles.size());

        // Bind VAO first
        vao->bind();

        // Upload vertex data to VBO
        vbo->bind();
        vbo->data(meshData.vertices.size() * sizeof(Vertex),
                  meshData.vertices.data(),
                  GL_STATIC_DRAW);

        // Upload index data to EBO
        ebo->bind();
        ebo->data(meshData.triangles.size() * sizeof(int),
                  meshData.triangles.data(),
                  GL_STATIC_DRAW);

        // Set up vertex attributes
        vao->setLayout<Vertex>();

        // Unbind VAO
        vao->unbind();

        setCallbacks();
    }

    void onUpdate(float dt) override {
        float time = app->getElapsedTime();
        modelTransform = Matrix4::rotateY(time * 30.0f);
    }

    void onRender(Renderer& renderer) override {
        Camera& cam = app->getCamera();
        float aspect = float(app->getWidth()) / float(app->getHeight());
        Matrix4 viewProjection = cam.getProjectionMatrix(aspect) * cam.getViewMatrix();

        // Combine model transform with view-projection
        Matrix4 mvp = viewProjection * modelTransform;

        renderer.submit(vao, indexCount, shader, mvp);
    }

private:

    static constexpr const char* VERTEX_PATH = "shaders/vertex.glsl";
    static constexpr const char* FRAGMENT_PATH = "shaders/fragment.glsl";

    Mesh cube = MeshFactory::cube();

    std::shared_ptr<GL::Shader> shader = std::make_shared<GL::Shader>(VERTEX_PATH, FRAGMENT_PATH);
    std::shared_ptr<GL::VertexArray> vao = std::make_shared<GL::VertexArray>();
    std::shared_ptr<GL::VertexBuffer> vbo = std::make_shared<GL::VertexBuffer>();
    std::shared_ptr<GL::ElementBuffer> ebo = std::make_shared<GL::ElementBuffer>();
    std::shared_ptr<int> indexCount = std::make_shared<int>(0);

    Matrix4 modelTransform = Matrix4::identity();
};

#pragma once

#include <glad/glad.h>
#include <memory>

#include "Layer.hpp"
#include "util/Vector.hpp"
#include "util/Pi.hpp"
#include "util/Matrix4.hpp"
#include "renderer/geometry/MeshFactory.hpp"
#include "renderer/geometry/Mesh.hpp"
#include "renderer/GL/GLObject.hpp"
#include "renderer/GL/Shader.hpp"

class Renderer {
public:

    Renderer() = default;
    ~Renderer() = default;

    void begin(int width, int height);
    void submit();
    void end();

    void draw(int width, int height);

    void submit(const std::shared_ptr<GL::VertexArray>& vao,
                const std::shared_ptr<int> indexCount,
                const std::shared_ptr<GL::Shader>& shader,
                const Matrix4& transform);

    void push(std::shared_ptr<Layer> layer);
private:
    // TODO: Render queue.

};

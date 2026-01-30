#include "Application.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

Application::Application(const Config& config) : window(config.width, config.height, config.title) {};

void Application::run() {

    while (!window.shouldClose()) {

        updateTime(static_cast<float>(glfwGetTime()));

        window.pollEvents();
        window.getDimensions(config.width, config.height);

        window.beginImGuiFrame();

        for (auto& layer : layers) {
            layer->onUpdate(deltaTime);
        }

        renderer.begin(config.width, config.height);

        for (auto& layer : layers) {
            layer->onRender(renderer);
        }

        window.endImGuiFrame();
        window.swapBuffers();
    }
}

void Application::close() {
    window.close();
}

const std::vector<std::unique_ptr<Layer>>& Application::getLayers() const {
    return layers;
}

void Application::updateTime(float now) {
    deltaTime = now - lastFrameTime;
    elapsedTime += deltaTime;
    lastFrameTime = now;
}

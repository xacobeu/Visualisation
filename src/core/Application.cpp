#include "Application.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>

Application::Application(int width, int height, const char* title) : window(width, height, title) {};

void Application::run() {

    while (!window.shouldClose()) {

        updateTime(static_cast<float>(glfwGetTime()));

        window.pollEvents();
        window.getDimensions(width, height);

        window.beginImGuiFrame();

        for (auto& layer : layers) {
            layer->onUpdate(deltaTime);
        }

        renderer.begin(width, height);

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

void Application::push(std::unique_ptr<Layer> layer) {
    layer->app = this;
    layer->onAttach();
    layers.emplace_back(std::move(layer));
}

void Application::updateTime(float now) {
    deltaTime = now - lastFrameTime;
    elapsedTime += deltaTime;
    lastFrameTime = now;
}

#pragma once

#include "renderer/Renderer.hpp"
#include "Window.hpp"
#include "renderer/Camera.hpp"

#include <memory>
#include <vector>

class Application {
public:

    int width, height;

    Application(int width = 800, int height = 600, const char* title = "");
    ~Application() = default;

    void run();
    void close();

    void push(std::unique_ptr<Layer> layer);

    Window::Events& getWindowEvents() { return window.events; }
    void getWindowDimensions(int& width, int& height) const { window.getDimensions(width, height); }
    int getWidth() const { return width; }
    int getHeight() const { return height; }

    float getElapsedTime() const { return elapsedTime; }
    float getDeltaTime() const { return deltaTime; }

    void updateTime(float now);

    Camera& getCamera() { return camera; }
    double getCursorX() const { return window.getCursorX(); }
    double getCursorY() const { return window.getCursorY(); }

private:
    Renderer renderer;
    Window window;
    Camera camera;

    std::vector<std::unique_ptr<Layer>> layers;

    float elapsedTime = 0.0f;
    float deltaTime = 0.0f;
    float lastFrameTime = 0.0f;
};

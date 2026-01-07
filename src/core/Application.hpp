#pragma once

#include "renderer/Renderer.hpp"
#include "Window.hpp"
#include "renderer/Camera.hpp"

#include <memory>
#include <vector>
#include <stdexcept>

struct Config {
    int width = 800;
    int height = 600;
    const char* title = "Application";
};

class Application {
public:

    Application(const Config& config);
    ~Application() = default;

    void run();
    void close();

    const std::vector<std::unique_ptr<Layer>>& getLayers() const;

    Window::Events& getWindowEvents() { return window.events; }
    void getWindowDimensions(int& width, int& height) const { window.getDimensions(width, height); }
    int getWidth() const { return config.width; }
    int getHeight() const { return config.height; }

    float getElapsedTime() const { return elapsedTime; }
    float getDeltaTime() const { return deltaTime; }

    void updateTime(float now);

    Camera& getCamera() { return camera; }
    double getCursorX() const { return window.getCursorX(); }
    double getCursorY() const { return window.getCursorY(); }

private:
    Config config;

    Renderer renderer;
    Window window;
    Camera camera;

    std::vector<std::unique_ptr<Layer>> layers;

    float elapsedTime = 0.0f;
    float deltaTime = 0.0f;
    float lastFrameTime = 0.0f;

public:

    template<typename T>
    requires std::is_base_of_v<Layer, T>
    void push(std::unique_ptr<T> layer) {

        if constexpr (UniqueLayer<T>) {
            if (getLayer<T>() != nullptr) {
                throw std::runtime_error("Duplicate unique layer was added to application.");
            }
        }

        layer->app = this;
        layer->onAttach();
        layers.emplace_back(std::move(layer));
    }

    template<typename T>
    T* getLayer() {

        for (auto& layer : layers) {
            if (auto ptr = dynamic_cast<T*>(layer.get())) {
                return ptr;
            }
        }

        return nullptr;
    }
};

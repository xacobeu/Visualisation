#pragma once

#include "renderer/Renderer.hpp"
#include "window/Window.hpp"

class App {
public:
    bool init();
    void run();

private:
    Window window {800, 600, "Visualisation"};
    Renderer renderer;
};

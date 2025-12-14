#include "App.hpp"
#include "renderer/Camera.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <cstdio>

// Initialize libraries and allat.
bool App::init() {

    // Initialize renderer
    if (!renderer.init()) return false;

    window.events.onMouseClick = [this](int button, int action, int mods) {

        if (button != GLFW_MOUSE_BUTTON_LEFT) return;
            
        Camera& cam = this->renderer.camera;
        if (action == GLFW_PRESS) {
            cam.dragging = true;
            cam.lastMouseX = window.getCursorX();
            cam.lastMouseY = window.getCursorY();
        } else if (action == GLFW_RELEASE) {
            cam.dragging = false;
        }
    };

    window.events.onScroll = [this](double xoffset, double yoffset) {       
        this->renderer.camera.zoom(float(yoffset));
    };

    window.events.onMouseMove = [this](double x, double y) {
        Camera& cam = this->renderer.camera;
        if (cam.dragging) {
            cam.update(x, y);
        }
    };

    return true;
}

// Main loop.
void App::run() {
    while (!window.shouldClose())
    {
        window.pollEvents();
        window.beginImGuiFrame();

        ImGui::ShowDemoWindow();
        ImGuiIO& io = ImGui::GetIO();
        
        renderer.render((int) io.DisplaySize.x, (int) io.DisplaySize.y);

        window.endImGuiFrame();
        window.swapBuffers();
    }
}

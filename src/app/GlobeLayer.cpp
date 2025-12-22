#include "GlobeLayer.hpp"

void GlobeLayer::setCallbacks() {
    app->getWindowEvents().onMouseClick = [this](int button, int action, int mods) {

        if (button != GLFW_MOUSE_BUTTON_LEFT) return;

        Camera& cam = app->getCamera();
        if (action == GLFW_PRESS) {
            cam.dragging = true;
            cam.lastMouseX = app->getCursorX();
            cam.lastMouseY = app->getCursorY();
        } else if (action == GLFW_RELEASE) {
            cam.dragging = false;
        }
    };

    app->getWindowEvents().onScroll = [this](double xoffset, double yoffset) {
        app->getCamera().zoom(float(yoffset));
    };

    app->getWindowEvents().onMouseMove = [this](double x, double y) {
        Camera& cam = app->getCamera();
        if (cam.dragging) {
            cam.update(x, y);
        }
    };

    app->getWindowEvents().onKey = [this](int key, int action, int mods) {
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            app->close();
        }
    };
}

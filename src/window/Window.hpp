#pragma once 

#include <functional>

struct GLFWwindow;

class Window {
public:

    Window(int width, int height, const char* title);
    ~Window();

    void initImGui();
    void beginImGuiFrame();
    void endImGuiFrame();

    struct Events {
        std::function<void(int button, int action, int mods)> onMouseClick;
        std::function<void(double x, double y)>               onMouseMove;
        std::function<void(double x, double y)>               onScroll;
        std::function<void(int key, int action, int mods)>    onKey;
        std::function<void(int width, int height)>            onResize;
    } events;

    bool shouldClose() const;
    void swapBuffers() const;
    void pollEvents() const;
    int getDimensions(int& width, int& height) const;

    bool isKeyPressed(int key) const;

    double getCursorX() const;
    double getCursorY() const;

    int getWidth() const;
    int getHeight() const;

private:
    GLFWwindow* window = nullptr;
    
    static void _mouseButtonCallback(GLFWwindow* win, int button, int action, int mods);
    static void _cursorPosCallback(GLFWwindow* win, double x, double y);
    static void _scrollCallback(GLFWwindow* win, double xoffset, double yoffset);
    static void _keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods);
    static void _charCallback(GLFWwindow* w, unsigned int codepoint);
    static void _resizeCallback(GLFWwindow* win, int width, int height);
};

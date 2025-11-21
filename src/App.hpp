#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "renderer/Renderer.hpp"

class App {
public:
    bool init();
    void run();
    void shutdown();

    GLFWwindow* window = nullptr;
    Renderer renderer;
};

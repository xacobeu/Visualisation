#include "App.hpp"
#include "renderer/Camera.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <cstdio>

// Initialize libraries and allat.
bool App::init() {
    // Init GLFW.
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    window = glfwCreateWindow(1280, 720, "Visualisation", nullptr, nullptr);
    if (!window) return false;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetWindowUserPointer(window, this);

    glfwSetScrollCallback(window, 
        [](GLFWwindow* win, double xoff, double yoff) {
            if (App* app = (App*)glfwGetWindowUserPointer(win)) {
                app->renderer.camera.zoom(float(yoff));
            }
        }
    );
    glfwSetMouseButtonCallback(window,
        [](GLFWwindow* win, int button, int action, int mods) {
            if (button != GLFW_MOUSE_BUTTON_LEFT) return;
            if (App* app = (App*) glfwGetWindowUserPointer(win)) {
                
                Camera& cam = app->renderer.camera;
                if (action == GLFW_PRESS) {
                    cam.dragging = true;
                    glfwGetCursorPos(win, &cam.lastMouseX, &cam.lastMouseY);
                } else if (action == GLFW_RELEASE) {
                    cam.dragging = false;
                }
            }
        }
    );
    glfwSetCursorPosCallback(window, 
        [](GLFWwindow* win, double x, double y) {
            if (App* app = (App*) glfwGetWindowUserPointer(win)) {
                Camera& cam = app->renderer.camera;
                if (cam.dragging) {
                    cam.update(x, y);
                }
            }
        }
    );

    // Load OpenGL.
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) return false;

    // Setup ImGui.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 450");

    // Initialize renderer
    if (renderer.init()) return true;
    
    return false;
}

// Main loop.
void App::run() {
    ImGuiIO& io = ImGui::GetIO();

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start ImGui.
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Show ImGui demo window.
        bool showDemo = true;
        ImGui::ShowDemoWindow(&showDemo);
        ImGui::Render();
  
        // OpenGL Render.
        glViewport(0, 0, (int) io.DisplaySize.x, (int) io.DisplaySize.y);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        
        renderer.render((int) io.DisplaySize.x, (int) io.DisplaySize.y);
        
        // ImGui Render.
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }
}

// Cleanup.
void App::shutdown() {
    renderer.cleanup();
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
}
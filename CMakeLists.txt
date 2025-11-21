cmake_minimum_required(VERSION 3.10)
project(ImGuiOpenGLProject)
set(CMAKE_CXX_STANDARD 17)

# GLFW
add_subdirectory(external/glfw)

# glad (build as static library)
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_SOURCE_DIR}/external/glad/include)

# ImGui
set(IMGUI_SRC
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui_tables.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui_widgets.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/imgui_impl_opengl3.cpp
)

add_library(imgui ${IMGUI_SRC})
target_include_directories(imgui PUBLIC ${CMAKE_SOURCE_DIR}/external/imgui)
target_include_directories(imgui PUBLIC ${CMAKE_SOURCE_DIR}/external/glfw/include)

# Use GLAD as the OpenGL loader for ImGui
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD)

# Propagate GLFW for imgui.
target_link_libraries(imgui PUBLIC glfw)

# Prefer CMake's OpenGL target.
find_package(OpenGL REQUIRED)

add_executable(${PROJECT_NAME} src/main.cpp src/App.cpp src/renderer/Renderer.cpp src/renderer/Shader.cpp src/renderer/Country.cpp src/renderer/Camera.cpp src/renderer/SphereGenerator.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/glad/include
    ${CMAKE_SOURCE_DIR}/external/imgui
    ${CMAKE_SOURCE_DIR}/external/glfw/include
    ${CMAKE_SOURCE_DIR}/external/stb_image
    ${CMAKE_SOURCE_DIR}/external/json
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui
    glad
    glfw
    OpenGL::GL
)

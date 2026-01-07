cmake_minimum_required(VERSION 3.25)

cmake_policy(SET CMP0072 NEW)
set(OpenGL_GL_PREFERENCE GLVND)

project(ImGuiOpenGLProject)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Detect vcpkg triplet based on platform
if(WIN32)
    set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "")
elseif(APPLE)
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(VCPKG_TARGET_TRIPLET "arm64-osx" CACHE STRING "")
    else()
        set(VCPKG_TARGET_TRIPLET "x64-osx" CACHE STRING "")
    endif()
else()
    set(VCPKG_TARGET_TRIPLET "x64-linux" CACHE STRING "")
endif()

# Set vcpkg installed directory as prefix path
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")

# Find packages installed by vcpkg
find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(implot CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(${PROJECT_NAME}
    src/app/Main.cpp
    src/app/layers/UILayer.cpp
    src/app/layers/MapLayer.cpp
    src/app/util/GraphRenderer.cpp
    src/app/util/DataHandler.cpp

    src/core/renderer/geometry/Mesh.cpp
    src/core/renderer/geometry/MeshFactory.cpp
    src/core/renderer/GL/Shader.cpp
    src/core/renderer/GL/Vertex.cpp
    src/core/renderer/Renderer.cpp
    src/core/renderer/Camera.cpp

    src/core/Application.cpp
    src/core/Window.cpp
)

# Ensure C++20 features are available to this target
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

# Compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:
        -Wall -Wextra -Wpedantic -Wold-style-cast -Wunused -Wcast-align -Wnon-virtual-dtor -Woverloaded-virtual
    >
)

target_include_directories(${PROJECT_NAME} PRIVATE
    src/app
    src/core
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui::imgui
    glfw
    glad::glad
    OpenGL::GL
    implot::implot
    nlohmann_json::nlohmann_json
)

# Copy required DLLs to output directory on Windows
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/bin/glfw3.dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()

# Copy res folder
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/res"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/res"
)

# Copy shaders folder
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
)


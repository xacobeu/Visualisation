cmake_minimum_required(VERSION 3.10)
project(ImGuiOpenGLProject)
set(CMAKE_CXX_STANDARD 17)

# Detect vcpkg triplet based on platform
if(WIN32)
    set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "")
elseif(APPLE)
    set(VCPKG_TARGET_TRIPLET "x64-osx" CACHE STRING "")
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
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(${PROJECT_NAME} 
    src/main.cpp 
    src/App.cpp 
    src/renderer/Renderer.cpp 
    src/renderer/Shader.cpp 
    src/renderer/Country.cpp 
    src/renderer/Camera.cpp 
    src/renderer/SphereGenerator.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    imgui::imgui
    glfw
    glad::glad
    OpenGL::GL
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
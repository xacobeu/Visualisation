# Geoverview

To run the application on windows, navigate to dist/bin/ and run ImGuiOpenGLProject.exe. For other operating systems or if this does not work, follow the instructions for developers below.

The application's source code is located in src/ while the rendering engine and rendering code are located in core/

## Datasets

Country border data: Made with Natural Earth. GeoJSON formatting from.
https://geojson-maps.kyd.au/

CIA Global Statistical Database for data points.
https://www.kaggle.com/datasets/kushagraarya10/cia-global-statistical-database

## Libraries

Third party code is installed with vcpkg and is located in vcpkg_installed folders 
(Follow steps for developers below to download libraries if needed)

- Dear ImGui with OpenGL Rendering backend - GUI Library
- ImPlot, Extension of ImGui for plotting and graphs
- GLAD, OpenGL loader
- GLFW, platform backend, creates window, handles input
- stb, image loading, loading textures
- nlohmann/json, JSON parsing
- earcut-hpp, Algorithm to convert GeoJSON country data to triangles.

## Tools Used

- vcpkg package manager
- CMake
- Visual Studio Code

## For developers

### Prerequisites

1. **Install a C++ Compiler**

2. **Install CMake** (if not already installed)

3. **Install vcpkg** (if not already installed)

4. **Install dependencies** (in the project root):
   ```powershell
   vcpkg install
   ```

### Running

1. **Install 'CMake Tools' VScode extension**

2. **Run 'Cmake: Configure' (Ctrl + Shift + p)**

3. **Run 'Cmake: Build' (Ctrl + Shift + p)**

4. **Run 'Cmake: Debug' (Ctrl + Shift + p)**


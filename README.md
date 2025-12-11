# Geography Visualisation Tool

## Prerequisites
- CMake 3.21 or higher
- [vcpkg](https://github.com/microsoft/vcpkg)
- C++17 compatible compiler

## Building

### Windows

1. **Install CMake** (if not already installed)
   - Download from [cmake.org](https://cmake.org/download/)

2. **Install vcpkg** (if not already installed):
   ```powershell
   cd C:\
   git clone https://github.com/microsoft/vcpkg.git
   .\vcpkg\bootstrap-vcpkg.bat
   ```

3. **Set environment variable for vcpkg**:
   ```powershell
   [System.Environment]::SetEnvironmentVariable('VCPKG_ROOT', 'C:\vcpkg', 'User')
   ```
   Then restart your terminal for the changes to take effect.

4. **Install dependencies** (in the project directory):
   ```powershell
   vcpkg install
   ```
   This reads `vcpkg.json` and installs glfw3, glad, imgui, and stb to `vcpkg_installed/x64-windows/`.

5. **Configure and build**:
   ```powershell
   cmake -B build -S .
   cmake --build build --config Release
   ```
   CMake automatically finds packages in the local `vcpkg_installed` directory.

6. **Run the application**:
   ```powershell
   .\build\Release\ImGuiOpenGLProject.exe
   ```

### macOS/Linux

1. **Install CMake** (if not already installed)
   
   macOS:
   ```bash
   brew install cmake
   ```
   
   Linux (Ubuntu/Debian):
   ```bash
   sudo apt update
   sudo apt install cmake build-essential
   ```

2. **Install vcpkg** (if not already installed):
   ```bash
   cd ~
   git clone https://github.com/microsoft/vcpkg.git
   ./vcpkg/bootstrap-vcpkg.sh
   ```

3. **Set environment variable for vcpkg**:
   
   Add to `~/.bashrc` or `~/.zshrc`:
   ```bash
   export VCPKG_ROOT=~/vcpkg
   export PATH=$VCPKG_ROOT:$PATH
   ```
   Then reload your shell:
   ```bash
   source ~/.bashrc  # or source ~/.zshrc
   ```

4. **Install dependencies** (in the project directory):
   ```bash
   vcpkg install
   ```
   This reads `vcpkg.json` and installs glfw3, glad, imgui, and stb to `vcpkg_installed/x64-osx/` or `vcpkg_installed/x64-linux/`.

5. **Configure and build**:
   ```bash
   cmake -B build -S .
   cmake --build build --config Release
   ```
   CMake automatically detects your platform and finds packages in the local `vcpkg_installed` directory.

6. **Run the application**:
   ```bash
   ./build/ImGuiOpenGLProject
   ```

## How It Works

This project uses vcpkg's **manifest mode** for dependency management:

- **`vcpkg.json`**: Declares all dependencies (glfw3, glad, imgui, stb)
- **`vcpkg install`**: Installs packages locally to `vcpkg_installed/<triplet>/`
- **`CMakeLists.txt`**: Automatically detects platform and configures CMAKE_PREFIX_PATH to find installed packages

This approach keeps dependencies project-local and works identically across Windows, macOS, and Linux without requiring manual toolchain file specification.


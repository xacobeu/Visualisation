#pragma once

#include <glad/glad.h>

#include "util/MathUtils.hpp"
#include "Country.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "util/Pi.hpp"

#include "mesh/MeshFactory.hpp"
#include "mesh/Mesh.hpp"
#include "GL/GLObject.hpp"

class Renderer {
public:

    static constexpr const char* GLOBE_VERT_PATH = "shaders/globeVertexShader.glsl";
    static constexpr const char* GLOBE_FRAG_PATH = "shaders/globeFragmentShader.glsl";
    static constexpr const char* BORDER_VERT_PATH = "shaders/borderVertexShader.glsl";
    static constexpr const char* BORDER_FRAG_PATH = "shaders/borderFragmentShader.glsl";
    static constexpr const char* SPACE_VERT_PATH  = "shaders/spaceVertexShader.glsl";
    static constexpr const char* SPACE_FRAG_PATH  = "shaders/spaceFragmentShader.glsl";

    static constexpr const char* PATH_TO_HEIGHTMAP = "res/textures/4k/heightmap.jpg";
    static constexpr const char* PATH_TO_COLORMAP = "res/textures/4k/colormap.jpg";

    Renderer();
    ~Renderer();

    bool init();
    void cleanup();
    void render(int width, int height);

    Camera camera;
    Mesh sphere;

private:

    const int SPHERE_RESOLUTION = 512;
    const float ELEVATION_SCALE = 0.0f;

    // Shaders
    Shader globeShader;
    Shader borderShader;
    Shader spaceBackgroundShader;

    // Globe mesh buffers
    GL::VertexArray globeVAO;
    GL::VertexBuffer globeVBO;
    GL::ElementBuffer globeEBO;
    int globeIndexCount = 0;

    // Borders mesh buffers
    GL::VertexArray borderVAO;
    GL::VertexBuffer borderVBO;
    int borderVertexCount = 0;

    // Space background quad buffers
    GL::VertexArray spaceVAO;
    GL::VertexBuffer spaceVBO;

    void setupGlobeBuffers(const Mesh::MeshData& mesh);
    void setupBorderBuffers();
    void setupSpaceBackgroundQuad();

    // Textures
    GLuint heightTexture = 0;
    GLuint colorTexture = 0;
    void setupHeightTexture();
    void setupColorTexture();

    // Country data
    std::vector<Country> countries;

    // Helpers
    float sampleHeight(Vector2 uv);
    bool loadMapData();

    struct mapData {
        int width;
        int height;
        unsigned char* data = nullptr;
    } heightmap, colormap;

    static void safeStbiFree(mapData& map);
    
    static inline float deg2rad(float d) { return d * PI / 180.0f; }

};

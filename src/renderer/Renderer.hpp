#pragma once

#include <glad/glad.h>

#include "util/MathUtils.hpp"
#include "util/MeshData.h"
#include "Country.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "SphereGenerator.hpp"
#include "util/PI.hpp"

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

private:

    const int SPHERE_RESOLUTION = 512;
    const float ELEVATION_SCALE = 0.0f;

    // Shaders
    Shader globeShader;
    Shader borderShader;
    Shader spaceBackgroundShader;

    // Textures
    GLuint heightTexture = 0;
    GLuint colorTexture = 0;
    void setupHeightTexture();
    void setupColorTexture();

    // Globe mesh buffers
    GLuint globeVAO = 0;
    GLuint globeVBO = 0;
    GLuint globeEBO = 0;
    int globeIndexCount = 0;
    void setupGlobeBuffers(MeshData mesh);

    // Borders mesh buffers
    GLuint borderVAO = 0;
    GLuint borderVBO = 0;
    int borderVertexCount = 0;
    void setupBorderBuffers();

    // Space background quad buffers
    GLuint spaceVAO = 0;
    GLuint spaceVBO = 0;
    void setupSpaceBackgroundQuad();

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

    static inline void safeDeleteVAO(GLuint& vao) { if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }};
    static inline void safeDeleteBuffer(GLuint& buf) { if (buf) { glDeleteBuffers(1, &buf); buf = 0; }};
    static inline void safeDeleteTexture(GLuint& tex) { if (tex) { glDeleteTextures(1, &tex); tex = 0; }};
    static void safeStbiFree(mapData& map);
    
    static inline float deg2rad(float d) { return d * PI / 180.0f; }

};
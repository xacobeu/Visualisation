#include "Renderer.hpp"
#include "util/Matrix4.hpp"



#include <vector>
#include <iostream>
#include <cmath>
#include <filesystem>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Renderer::Renderer() { }
Renderer::~Renderer() { cleanup(); }

bool Renderer::init() {

    printf("Initializing renderer...\n");
    
    // Load map data
    if (!loadMapData()) return false;

    // Globe setup
    setupHeightTexture();
    setupColorTexture();

    sphere = MeshFactory::sphere(SPHERE_RESOLUTION, Vector3{0.0f, 0.0f, 0.0f});
    setupGlobeBuffers(sphere.getData());

    if (!globeShader.compileFromFiles(GLOBE_VERT_PATH, GLOBE_FRAG_PATH)) {
        std::cerr << "Failed to create globe shader program.\n";
        return false;
    }
    printf("Globe shader compiled successfully.\n");

    // Borders setup
    setupBorderBuffers();
    if (!borderShader.compileFromFiles(BORDER_VERT_PATH, BORDER_FRAG_PATH)) {
        std::cerr << "Failed to create border shader program.\n";
        return false;
    }
    printf("Border shader compiled successfully.\n");

    // Space background setup
    setupSpaceBackgroundQuad();
    if (!spaceBackgroundShader.compileFromFiles(SPACE_VERT_PATH, SPACE_FRAG_PATH)) {
        std::cerr << "Failed to create space background shader program.\n";
        return false;
    }
    printf("Space background shader compiled successfully.\n");
    
    return true;
}

void Renderer::cleanup() {
    // Free map data
    safeStbiFree(heightmap);
    safeStbiFree(colormap);
}

void Renderer::render(int width, int height) {

    // === Render space background ===
    // Disable depth testing for background
    glDisable(GL_DEPTH_TEST);
    
    spaceBackgroundShader.bind();
    spaceBackgroundShader.setVec2("uResolution", (float)width, (float)height);
    
    spaceVAO.bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    spaceVAO.unbind();
        
    glEnable(GL_DEPTH_TEST);
    
    // Get camera matrices
    Matrix4 view = camera.getViewMatrix();
    Matrix4 proj = camera.getProjectionMatrix(float(width) / float(height), 45.0f);


    // === Render globe ===
    globeShader.bind();
    globeShader.setMat4("uView", view.data());
    globeShader.setMat4("uProj", proj.data());    

    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, heightTexture);
    globeShader.setInt("heightmap", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    globeShader.setInt("colormap", 1);

    // Draw.
    globeVAO.bind();
    glDrawElements(GL_TRIANGLES, globeIndexCount, GL_UNSIGNED_INT, 0);
    globeVAO.unbind();
    
    Shader::unbind();

    // === Render borders ===
    borderShader.bind();
    borderShader.setMat4("uView", view.data());
    borderShader.setMat4("uProj", proj.data());
    
    borderVAO.bind();
    glDrawArrays(GL_LINES, 0, borderVertexCount);
    borderVAO.unbind();
    
    Shader::unbind();
}

// === GPU stuff ===

void Renderer::setupGlobeBuffers(const Mesh::MeshData& mesh) {

    globeVAO.create();
    globeVBO.create();
    globeEBO.create();
    
    globeIndexCount = static_cast<int>(mesh.triangles.size());
    
    globeVAO.bind();
    
    {
        globeVBO.bind();
        globeVBO.data(mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), GL_STATIC_DRAW);

        globeEBO.bind();
        globeEBO.data(mesh.triangles.size() * sizeof(int), mesh.triangles.data(), GL_STATIC_DRAW);

        globeVAO.setLayout<Vertex>();
    }

    globeVAO.unbind();

}

void Renderer::setupBorderBuffers() {
    if (!loadCountriesJson("res/countries_clean.json", countries)) {
        std::cerr << "Failed to load countries_clean.json\n";
        return;
    }

    struct BorderVertex {
        Vector3 pos;
    };

    std::vector<BorderVertex> verts;

    // For each country and each ring
    for (const auto& c : countries) {
        for (const auto& ring : c.borders) {
            if (ring.size() < 2) continue;

            // Construct a polyline on the sphere
            for (size_t i = 0; i < ring.size(); ++i) {
                const Vector2& ll0 = ring[i];
                const Vector2& ll1 = ring[(i + 1) % ring.size()];

                float lon0 = deg2rad(ll0.x);
                float lat0 = deg2rad(ll0.y);

                float lon1 = deg2rad(ll1.x);
                float lat1 = deg2rad(ll1.y);

                Vector3 p0 {
                    -cosf(lat0) * cosf(lon0),
                    sinf(lat0),
                    cosf(lat0) * sinf(lon0)
                };
                Vector3 p1 {
                    -cosf(lat1) * cosf(lon1),
                    sinf(lat1),
                    cosf(lat1) * sinf(lon1)
                };

                float borderRadius = 1.001f;
                p0 = p0 * borderRadius;
                p1 = p1 * borderRadius;

                verts.push_back({ p0 });
                verts.push_back({ p1 });
            }
        }
    }

    borderVertexCount = (int)verts.size();
    if (borderVertexCount == 0) return;

    borderVAO.create();
    borderVBO.create();

    borderVAO.bind();
    borderVBO.bind();
    borderVBO.data(verts.size() * sizeof(BorderVertex), verts.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(BorderVertex), (void*)0);
    glEnableVertexAttribArray(0);

    borderVAO.unbind();

    std::cout << "Border vertices: " << borderVertexCount << "\n";
}

void Renderer::setupHeightTexture() {
    glGenTextures(1, &heightTexture);
    glBindTexture(GL_TEXTURE_2D, heightTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, heightmap.width, heightmap.height, 0, GL_RED, GL_UNSIGNED_BYTE, heightmap.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void Renderer::setupColorTexture() {
    glGenTextures(1, &colorTexture);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, colormap.width, colormap.height, 0, GL_RGB, GL_UNSIGNED_BYTE, colormap.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}



float Renderer::sampleHeight(Vector2 uv) {
    // Wrap UV
    uv.x = uv.x - floorf(uv.x);
    uv.y = uv.y - floorf(uv.y);

    int x = int(uv.x * heightmap.width);
    int y = int(uv.y * heightmap.height);

    // Clamp
    if (x < 0) x = 0;
    else if (x > heightmap.width - 1) x = heightmap.width - 1;

    if (y < 0) y = 0;
    else if (y > heightmap.height - 1) y = heightmap.height - 1;

    unsigned char px = heightmap.data[y * heightmap.width + x];

    return px / 255.0f;
}

bool Renderer::loadMapData() {

    printf("Loading map data...\n");

    int w, h, c;

    // Load heightmap (grayscale)
    unsigned char* img = stbi_load(PATH_TO_HEIGHTMAP, &w, &h, &c, 1);
    if (!img) {
        std::cerr << "Failed to load heightmap image\n";
        std::cerr << stbi_failure_reason() << "\n";
        return false;
    }

    heightmap.width = w;
    heightmap.height = h;
    heightmap.data = img;

    // Load colormap (RGB)
    unsigned char* cmap = stbi_load(PATH_TO_COLORMAP, &w, &h, &c, 3);
    if (!cmap) {
        std::cerr << "Failed to load colormap\n";
        std::cerr << stbi_failure_reason() << "\n";
        return false;
    }

    colormap.width = w;
    colormap.height = h;
    colormap.data = cmap;

    return true;
}

void Renderer::setupSpaceBackgroundQuad() {
    // Full-screen quad vertices (NDC coordinates)
    float quadVertices[] = {
        -1.0f, -1.0f,  // Bottom-left
         1.0f, -1.0f,  // Bottom-right
         1.0f,  1.0f,  // Top-right
        -1.0f,  1.0f   // Top-left
    };

    unsigned int quadIndices[] = {
        0, 1, 2,
        2, 3, 0
    };

    spaceVAO.create();
    spaceVBO.create();
    GLuint spaceEBO;
    glGenBuffers(1, &spaceEBO);

    spaceVAO.bind();

    spaceVBO.bind();
    spaceVBO.data(sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, spaceEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    spaceVAO.unbind();
}

void Renderer::safeStbiFree(mapData& map) { 
    if (map.data) { 
        stbi_image_free(map.data); 
        map.data = nullptr; 
    }
}

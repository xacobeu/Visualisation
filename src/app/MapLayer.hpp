#pragma once

#include <memory>
#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include <array>

#include <nlohmann/json.hpp>
#include <mapbox/earcut.hpp>

#include "Layer.hpp"
#include "Application.hpp"
#include "renderer/GL/GLObject.hpp"
#include "renderer/GL/Shader.hpp"
#include "renderer/Renderer.hpp"
#include "renderer/Camera.hpp"

struct MapVertex {
    Vector3 position;
    Vector3 normal;
    Vector4 color; // RGBA color

    struct Attribute {
        GLuint index;
        GLint  size;
        GLenum type;
        std::size_t offset;
    };

    static const std::array<Attribute, 3> attributes;
};

// 2. Helper for Earcut (Triangulation)
using Point = std::array<double, 2>;
namespace mapbox { namespace util {
    template <> struct nth<0, Point> { inline static double get(const Point &t) { return t[0]; }; };
    template <> struct nth<1, Point> { inline static double get(const Point &t) { return t[1]; }; };
}}

class MapLayer : public Layer {
    struct CountryMetadata {
        std::string name;
        std::vector<size_t> globalVertexIndices;
        std::vector<std::vector<Point>> rawPolygons;

        // Bounding Box for optimization
        double minX = 1000, maxX = -1000, minY = 1000, maxY = -1000;

        void updateBounds(double x, double y) {
            if(x < minX) minX = x; if(x > maxX) maxX = x;
            if(y < minY) minY = y; if(y > maxY) maxY = y;
        }

        bool contains(double x, double y) const {
            return x >= minX && x <= maxX && y >= minY && y <= maxY;
        }
    };

public:
    MapLayer(const std::string& geoJsonPath = GEOJSON_PATH) : mapFilePath(geoJsonPath) {}

    void onAttach() override;
    void onUpdate(float dt) override;
    void onRender(Renderer& renderer) override;

    void setCountryColor(const std::string& isoCode, float r, float g, float b, float a = 1.0f);

    // Selection access methods
    const std::unordered_set<std::string>& getSelectedCountries() const { return selectedCountries; }
    std::string getCountryName(const std::string& isoCode) const {
        auto it = countries.find(isoCode);
        return (it != countries.end()) ? it->second.name : "Unknown";
    }
    void deselectCountry(const std::string& isoCode) {
        if (selectedCountries.find(isoCode) != selectedCountries.end()) {
            selectedCountries.erase(isoCode);
            setCountryColor(isoCode, defaultColor.x, defaultColor.y, defaultColor.z, defaultColor.w);
        }
    }
    void clearSelection() {
        for (const auto& countryId : selectedCountries) {
            setCountryColor(countryId, defaultColor.x, defaultColor.y, defaultColor.z, defaultColor.w);
        }
        selectedCountries.clear();
    }

private:

    std::string mapFilePath;
    std::vector<MapVertex> vertices;
    std::vector<uint32_t> indices;
    std::unordered_map<std::string, CountryMetadata> countries;

    // Selection tracking (multiple countries can be selected)
    std::unordered_set<std::string> selectedCountries;
    Vector4 defaultColor = {0.1f, 0.6f, 0.4f, 1.0f};
    Vector4 selectedColor = {0.1f, 0.5f, 0.4f, 1.0f};

    // Engine Resources
    static constexpr const char* VERTEX_PATH = "shaders/mapvert.glsl";
    static constexpr const char* FRAGMENT_PATH = "shaders/mapfrag.glsl";
    static constexpr const char* GEOJSON_PATH = "res/data/custom.geo.json";

    std::shared_ptr<GL::Shader> shader = std::make_shared<GL::Shader>(VERTEX_PATH, FRAGMENT_PATH);
    std::shared_ptr<GL::VertexArray> vao = std::make_shared<GL::VertexArray>();
    std::shared_ptr<GL::VertexBuffer> vbo = std::make_shared<GL::VertexBuffer>();
    std::shared_ptr<GL::ElementBuffer> ebo = std::make_shared<GL::ElementBuffer>();
    std::shared_ptr<int> indexCount = std::make_shared<int>(0);

    Matrix4 modelTransform = Matrix4::identity();
    bool needsBufferUpdate = false;

    // Camera panning offset for infinite tiling
    float panOffsetX = 0.0f;
    float panOffsetY = 0.0f;
    static constexpr float MAP_WIDTH = 20.0f; // World space width of one map instance (-10 to 10)

    // Track initial mouse press position to distinguish click from drag
    double mousePressX = 0.0;
    double mousePressY = 0.0;

    void loadAndTriangulate(const std::string& path);
    void uploadBuffers();
    void handleMousePicking();
    void setupCallbacks();
};

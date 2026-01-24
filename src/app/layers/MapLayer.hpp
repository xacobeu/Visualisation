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
    Vector4 color;

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

class MapLayer : public Layer, public Unique {
    struct CountryMetadata {
        std::string name;
        std::vector<size_t> globalVertexIndices;
        std::vector<std::vector<Point>> rawPolygons;

        // Bounding Box
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
            // Use choropleth color if active, otherwise use default green color
            if (choroplethActive && choroplethColors.count(isoCode)) {
                const Vector4& col = choroplethColors.at(isoCode);
                setCountryColor(isoCode, col.x, col.y, col.z, col.w);
            } else {
                const Vector4& col = countryColors.count(isoCode) ? countryColors.at(isoCode) : DEFAULT_COUNTRY_COLOR;
                setCountryColor(isoCode, col.x, col.y, col.z, col.w);
            }
        }
    }
    void clearSelection() {
        for (const auto& countryId : selectedCountries) {
            // Use choropleth color if active, otherwise use default green color
            if (choroplethActive && choroplethColors.count(countryId)) {
                const Vector4& col = choroplethColors.at(countryId);
                setCountryColor(countryId, col.x, col.y, col.z, col.w);
            } else {
                const Vector4& col = countryColors.count(countryId) ? countryColors.at(countryId) : DEFAULT_COUNTRY_COLOR;
                setCountryColor(countryId, col.x, col.y, col.z, col.w);
            }
        }
        selectedCountries.clear();
    }
    void selectAll() {
        for (const auto& [id, meta] : countries) {
            if (selectedCountries.find(id) == selectedCountries.end()) {
                selectedCountries.insert(id);
                setCountryColor(id, SELECTED_COUNTRY_COLOR.x, SELECTED_COUNTRY_COLOR.y, SELECTED_COUNTRY_COLOR.z, SELECTED_COUNTRY_COLOR.w);
            }
        }
    }
    
    // Choropleth methods
    void applyChoropleth(const std::unordered_map<std::string, float>& countryValues, const std::string& unit = "");
    void clearChoropleth();
    bool isChoroplethActive() const { return choroplethActive; }
    const std::string& getChoroplethUnit() const { return choroplethUnit; }
    
    // Hover detection - returns country ISO code under cursor, or empty string
    std::string getCountryAtCursor() const;
    
    // Get choropleth value for a country (returns false if no data)
    bool getChoroplethValue(const std::string& isoCode, float& outValue) const {
        auto it = choroplethValues.find(isoCode);
        if (it != choroplethValues.end()) {
            outValue = it->second;
            return true;
        }
        return false;
    }
    
    // Get all country ISO codes
    std::vector<std::string> getAllCountryIsoCodes() const {
        std::vector<std::string> codes;
        for (const auto& [id, meta] : countries) {
            codes.push_back(id);
        }
        return codes;
    }
    
    // Get ISO code from country name
    std::string getIsoCodeFromName(const std::string& name) const {
        for (const auto& [id, meta] : countries) {
            if (meta.name == name) return id;
        }
        return "";
    }
    
    // Selection control
    void setSelectionEnabled(bool enabled) { selectionEnabled = enabled; }
    bool isSelectionEnabled() const { return selectionEnabled; }

private:

    struct ViewMetrics {
        float width;
        float height;
        float zoom;
        float aspect;
    };

    static constexpr float MAP_HALF_HEIGHT = 5.0f;
    static constexpr float MAP_HALF_WIDTH = 10.0f;
    static constexpr float MAP_WIDTH = 20.0f;
    static constexpr float BASE_CAM_DIST = 15.0f;

    ViewMetrics calculateViewMetrics() const;
    std::pair<float, float> screenToWorld(double x, double y, const ViewMetrics& vm) const;
    std::pair<float, float> latLonToWorld(double lon, double lat) const;
    void constrainPanY(float viewHeight);

    std::string mapFilePath;
    std::vector<MapVertex> vertices;
    std::vector<uint32_t> indices;
    std::unordered_map<std::string, CountryMetadata> countries;

    // Selection tracking (multiple countries can be selected)
    std::unordered_set<std::string> selectedCountries;
    static constexpr const Vector4 DEFAULT_COUNTRY_COLOR = {0.4f, 0.6f, 0.4f, 1.0f};
    static constexpr const Vector4 SELECTED_COUNTRY_COLOR = {0.886f, 0.714f, 1.0f, 1.0f};
    static constexpr const Vector4 NO_DATA_COUNTRY_COLOR = {0.3f, 0.3f, 0.3f, 1.0f};
    
    // Default country colors
    std::unordered_map<std::string, Vector4> countryColors;  // Base colors for each country
    
    // Choropleth state
    bool choroplethActive = false;
    std::unordered_map<std::string, Vector4> choroplethColors;  // Colors when choropleth is active
    std::unordered_map<std::string, float> choroplethValues;    // Raw values for tooltips
    std::string choroplethUnit;  // Unit for choropleth values (for tooltip display)
    
    // Selection enabled state
    bool selectionEnabled = true;

    Vector4 valueToColor(float normalized) const;  // Maps 0-1 to color gradient

    // Engine Resources
    static constexpr const char* VERTEX_PATH = "shaders/mapvert.glsl";
    static constexpr const char* FRAGMENT_PATH = "shaders/mapfrag.glsl";
    static constexpr const char* GEOJSON_PATH = "res/data/10m.json";

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

    // Track initial mouse press position to distinguish click from drag
    double mousePressX = 0.0;
    double mousePressY = 0.0;

    void loadAndTriangulate(const std::string& path);
    void uploadBuffers();
    void handleMousePicking();
    void setupCallbacks();
};

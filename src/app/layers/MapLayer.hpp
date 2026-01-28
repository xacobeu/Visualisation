#pragma once

#include "Layer.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <array>
#include <memory>
#include <functional>

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
        int index;
        int size;
        unsigned int type;
        size_t offset;
    };
    static const std::array<Attribute, 3> attributes;
};

class MapLayer : public Layer {
public:
    MapLayer() = default;
    virtual ~MapLayer() = default;

    void onAttach() override;
    void onUpdate(float deltaTime) override;
    void onRender(Renderer& renderer) override;
    
    void setHover(const std::string& isoCode);

    void setSelectionEnabled(bool enabled) { selectionEnabled = enabled; }
    void selectCountry(const std::string& isoCode);
    void deselectCountry(const std::string& isoCode);
    
    void clearSelection() {
        selectedCountries.clear();
        for (const auto& [id, meta] : countries) {
            updateCountryColor(id);
        }
    }

    void selectAll() {
        for (const auto& [id, meta] : countries) {
            selectedCountries.insert(id);
            updateCountryColor(id);
        }
    }

    const std::unordered_set<std::string>& getSelectedCountries() const { return selectedCountries; }
    
    // --- Choropleth Logic ---
    void applyChoropleth(const std::unordered_map<std::string, float>& countryValues, const std::string& unit, bool isDiverging);
    void clearChoropleth();
    bool isChoroplethActive() const { return choroplethActive; }
    
    bool getChoroplethValue(const std::string& iso, float& outVal) const {
        if (!choroplethActive) return false;
        auto it = choroplethValues.find(iso);
        if (it != choroplethValues.end()) {
            outVal = it->second;
            return true;
        }
        return false;
    }
    
    const std::string& getChoroplethUnit() const { return choroplethUnit; }

    // --- Helpers ---
    std::string getCountryName(const std::string& isoCode) const {
        auto it = countries.find(isoCode);
        if (it != countries.end()) return it->second.name;
        return isoCode;
    }

    std::string getIsoCodeFromName(const std::string& name) const {
        for (const auto& [iso, meta] : countries) {
            if (meta.name == name) return iso;
        }
        return "";
    }

    std::string getCountryAtCursor() const;

    struct Point { double x, y; };

private:

    struct CountryMetadata {
        std::string name;
        std::vector<std::vector<Point>> rawPolygons;
        std::vector<size_t> globalVertexIndices;
        
        float minX = 1e9, maxX = -1e9, minY = 1e9, maxY = -1e9;
        void updateBounds(float x, float y) {
            if(x < minX) minX = x; if(x > maxX) maxX = x;
            if(y < minY) minY = y; if(y > maxY) maxY = y;
        }
        bool contains(float x, float y) const {
            return x >= minX && x <= maxX && y >= minY && y <= maxY;
        }
    };

    struct ViewMetrics {
        float width;
        float height;
        float zoom;
        float aspect;
    };

    void loadAndTriangulate(const std::string& path);
    void uploadBuffers();
    void setupCallbacks();
    void handleMousePicking();
    void constrainPanY(float viewHeight);
    ViewMetrics calculateViewMetrics() const;
    std::pair<float, float> screenToWorld(double x, double y, const ViewMetrics& vm) const;
    std::pair<float, float> latLonToWorld(double lon, double lat) const;
    
    // Core color logic
    void updateCountryColor(const std::string& isoCode);
    void setCountryColor(const std::string& isoCode, float r, float g, float b, float a);
    
    Vector4 valueToColor(float normalized) const;

    std::string mapFilePath = "res/data/10m.json"; 
    std::unordered_map<std::string, CountryMetadata> countries;
    std::unordered_set<std::string> selectedCountries;
    std::unordered_set<std::string> highlightedCountries;
    std::string hoveredIso = "";

    std::vector<MapVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<MapVertex> borderVertices;
    std::vector<uint32_t> borderIndices;

    // These rely on the forward declarations above
    std::shared_ptr<GL::VertexArray> vao;
    std::shared_ptr<GL::VertexBuffer> vbo;
    std::shared_ptr<GL::ElementBuffer> ebo;

    std::shared_ptr<GL::VertexArray> borderVao;
    std::shared_ptr<GL::VertexBuffer> borderVbo;
    std::shared_ptr<GL::ElementBuffer> borderEbo;
    
    std::shared_ptr<GL::Shader> shader; 

    float panOffsetX = 0.0f;
    float panOffsetY = 0.0f;
    bool selectionEnabled = true;
    bool needsBufferUpdate = false;
    
    double mousePressX = 0.0;
    double mousePressY = 0.0;

    bool choroplethActive = false;
    std::unordered_map<std::string, Vector4> choroplethColors;
    std::unordered_map<std::string, float> choroplethValues;
    std::string choroplethUnit;

    static constexpr float BASE_CAM_DIST = 1000.0f;
    static constexpr float MAP_WIDTH = 3600.0f;
    static constexpr float MAP_HEIGHT = 1800.0f;
    static constexpr float MAP_HALF_WIDTH = 1800.0f;
    static constexpr float MAP_HALF_HEIGHT = 900.0f;
};
#include "MapLayer.hpp"
#include "imgui.h"
#include "implot.h"

#include <nlohmann/json.hpp>
#include <mapbox/earcut.hpp>

#include <limits>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>

// --- COLORS ---
const Vector4 BORDER_COLOR           = {0.0f, 0.0f, 0.0f, 1.0f}; // Black
const Vector4 DEFAULT_COUNTRY_COLOR  = {0.4f, 0.7f, 0.4f, 1.0f}; // Light Green
const Vector4 SELECTED_COUNTRY_COLOR = {0.2f, 0.5f, 0.2f, 1.0f}; // Dark Green (Click Selection)
const Vector4 HIGHLIGHT_COLOR        = {1.0f, 0.8f, 0.0f, 1.0f}; // Gold (Search Highlight)
const Vector4 NO_DATA_COUNTRY_COLOR  = {0.5f, 0.5f, 0.5f, 1.0f}; // Grey

const std::array<MapVertex::Attribute, 3> MapVertex::attributes = {{
    {0, 3, GL_FLOAT, offsetof(MapVertex, position)},
    {1, 3, GL_FLOAT, offsetof(MapVertex, normal)},
    {2, 4, GL_FLOAT, offsetof(MapVertex, color)}
}};

namespace mapbox {
    namespace util {
        template <>
        struct nth<0, MapLayer::Point> {
            inline static double get(const MapLayer::Point &t) { return t.x; };
        };
        template <>
        struct nth<1, MapLayer::Point> {
            inline static double get(const MapLayer::Point &t) { return t.y; };
        };
    }
}

MapLayer::ViewMetrics MapLayer::calculateViewMetrics() const {
    Camera& cam = app->getCamera();
    float h = (float)app->getHeight();
    if (h < 1.0f) h = 1.0f;
    float aspect = (float)app->getWidth() / h;

    ViewMetrics vm;
    vm.aspect = aspect;
    vm.zoom = cam.distance / BASE_CAM_DIST;
    vm.height = MAP_HALF_HEIGHT * vm.zoom;
    vm.width = vm.height * aspect;
    return vm;
}

std::pair<float, float> MapLayer::screenToWorld(double x, double y, const ViewMetrics& vm) const {
    float ndcX = (2.0f * (float)x / app->getWidth()) - 1.0f;
    float ndcY = 1.0f - (2.0f * (float)y / app->getHeight());
    float worldX = ndcX * vm.width + panOffsetX;
    float worldY = ndcY * vm.height + panOffsetY;
    return {worldX, worldY};
}

std::pair<float, float> MapLayer::latLonToWorld(double lon, double lat) const {
    float x = (float)(lon / 180.0 * MAP_HALF_WIDTH);
    float y = (float)(lat / 90.0 * MAP_HALF_HEIGHT);
    return {x, y};
}

void MapLayer::constrainPanY(float viewHeight) {
    float minY = -MAP_HALF_HEIGHT + viewHeight;
    float maxY = MAP_HALF_HEIGHT - viewHeight;
    if (minY > maxY) {
        panOffsetY = 0.0f;
    } else {
        if (panOffsetY < minY) panOffsetY = minY;
        if (panOffsetY > maxY) panOffsetY = maxY;
    }
}

void MapLayer::setHover(const std::string& isoCode) {
    if (hoveredIso != isoCode) {
        std::string previousHover = hoveredIso;
        hoveredIso = isoCode;
        if (!previousHover.empty()) {
            updateCountryColor(previousHover);
        }
        if (!hoveredIso.empty()) {
            updateCountryColor(hoveredIso);
        }
    }
}

void MapLayer::onAttach() {
    // Initialize buffers using GL namespace
    vao = std::make_shared<GL::VertexArray>();
    vbo = std::make_shared<GL::VertexBuffer>();
    ebo = std::make_shared<GL::ElementBuffer>();

    borderVao = std::make_shared<GL::VertexArray>();
    borderVbo = std::make_shared<GL::VertexBuffer>();
    borderEbo = std::make_shared<GL::ElementBuffer>();
    
    // Initialize Shader (Map Shader with vertex and fragment files)
    shader = std::make_shared<GL::Shader>();
    shader->compileFromFiles("shaders/mapvert.glsl", "shaders/mapfrag.glsl");

    vao->create();
    vbo->create();
    ebo->create();

    borderVao->create();
    borderVbo->create();
    borderEbo->create();

    loadAndTriangulate(mapFilePath);
    uploadBuffers();

    vao->bind();
    vbo->bind();
    vao->setLayout<MapVertex>(); 
    vao->unbind();

    borderVao->bind();
    borderVbo->bind(); 
    borderVao->setLayout<MapVertex>();
    borderVao->unbind();

    for (auto& [id, meta] : countries) {
        updateCountryColor(id);
    }
    uploadBuffers();

    Camera& cam = app->getCamera();
    cam.distance = BASE_CAM_DIST;
    cam.pitch = 0.0f;
    cam.yaw = 0.0f;

    setupCallbacks();
}

Vector4 MapLayer::valueToColor(float normalized) const {
    normalized = std::max(0.0f, std::min(1.0f, normalized));
    float r = 0.8f - normalized * 0.7f;
    float g = 0.9f - normalized * 0.7f;
    float b = 1.0f - normalized * 0.4f;
    return {r, g, b, 1.0f};
}

// --- COLOR PRIORITY LOGIC ---
void MapLayer::updateCountryColor(const std::string& isoCode) {

    Vector4 baseColor;
    
    // Search bar highlight - highest priority
    if (isoCode == searchHighlightIso) {
        baseColor = {1.0f, 1.0f, 0.0f, 1.0f}; // Bright yellow
        setCountryColor(isoCode, baseColor.x, baseColor.y, baseColor.z, baseColor.w);
        return;
    }
    
    // Choropleth
    if (choroplethActive) {
        if (choroplethColors.count(isoCode)) {
            baseColor = choroplethColors[isoCode];
        } else {
            baseColor = NO_DATA_COUNTRY_COLOR;
        }
    }
    else if (highlightedCountries.find(isoCode) != highlightedCountries.end()) {
        baseColor = HIGHLIGHT_COLOR;
    }
    else if (selectedCountries.find(isoCode) != selectedCountries.end()) {
        baseColor = SELECTED_COUNTRY_COLOR;
    }
    else {
        baseColor = DEFAULT_COUNTRY_COLOR;
    }

    // Hover effect -> lighten color
    if (isoCode == hoveredIso) {
        float lightenFactor = 0.4f;
        Vector4 hoverColor = {
            baseColor.x + (1.0f - baseColor.x) * lightenFactor,
            baseColor.y + (1.0f - baseColor.y) * lightenFactor,
            baseColor.z + (1.0f - baseColor.z) * lightenFactor,
            baseColor.w
        };
        setCountryColor(isoCode, hoverColor.x, hoverColor.y, hoverColor.z, hoverColor.w);
        return;
    }

    // Apply the base color
    setCountryColor(isoCode, baseColor.x, baseColor.y, baseColor.z, baseColor.w);
}

void MapLayer::applyChoropleth(const std::unordered_map<std::string, float>& countryValues, const std::string& unit, bool isDiverging) {
    if (countryValues.empty()) return;

    // Calculate Min/Max
    float minVal = std::numeric_limits<float>::max();
    float maxVal = std::numeric_limits<float>::lowest();

    for (const auto& [isoCode, value] : countryValues) {
        if (countries.find(isoCode) != countries.end()) {
            minVal = std::min(minVal, value);
            maxVal = std::max(maxVal, value);
        }
    }

    // Prepare Sequential Range
    float rangeSeq = maxVal - minVal;
    if (rangeSeq < 0.0001f) rangeSeq = 1.0f;

    choroplethActive = true;
    choroplethColors.clear();
    choroplethValues = countryValues;
    choroplethUnit = unit;

    // Select Colormap
    ImPlotColormap mapId = isDiverging ? ImPlotColormap_RdBu : ImPlotColormap_Viridis;

    // Apply Colors
    for (const auto& [id, meta] : countries) {
        Vector4 col;
        auto it = countryValues.find(id);
        
        if (it != countryValues.end()) {
            float val = it->second;
            float normalized = 0.0f;

            if (isDiverging) {
                if (val < 0.0f) {
                    // Negative side: Map [minVal, 0] -> [0.0, 0.5]
                    if (std::abs(minVal) > 0.0001f) {
                        float ratio = (val - minVal) / (0.0f - minVal);
                        normalized = ratio * 0.5f;
                    } else {
                        normalized = 0.5f;
                    }
                } else {
                    // Positive side: Map [0, maxVal] -> [0.5, 1.0]
                    if (maxVal > 0.0001f) {
                        float ratio = val / maxVal;
                        normalized = 0.5f + (ratio * 0.5f);
                    } else {
                        normalized = 0.5f;
                    }
                }
            } else {
                // Map [minVal, maxVal] -> [0.0, 1.0]
                normalized = (val - minVal) / rangeSeq;
            }

            // Clamp to safety
            normalized = std::max(0.0f, std::min(1.0f, normalized));

            ImVec4 imColor = ImPlot::SampleColormap(normalized, mapId);
            col = {imColor.x, imColor.y, imColor.z, imColor.w};
        } else {
            col = NO_DATA_COUNTRY_COLOR;
        }
        
        choroplethColors[id] = col;
        updateCountryColor(id);
    }
}

void MapLayer::clearChoropleth() {
    choroplethActive = false;
    choroplethColors.clear();
    choroplethValues.clear();
    choroplethUnit.clear();
    
    for (const auto& [id, meta] : countries) {
        updateCountryColor(id);
    }
}

void MapLayer::selectCountry(const std::string& isoCode) {
    if (countries.find(isoCode) == countries.end()) return;
    selectedCountries.insert(isoCode);
    updateCountryColor(isoCode);
}

void MapLayer::deselectCountry(const std::string& isoCode) {
    if (selectedCountries.find(isoCode) == selectedCountries.end()) return;
    selectedCountries.erase(isoCode);
    highlightedCountries.erase(isoCode);
    updateCountryColor(isoCode);
}

void MapLayer::onUpdate(float) {
    if (needsBufferUpdate) {
        uploadBuffers();
    }
}

void MapLayer::onRender(Renderer&) {

    ViewMetrics vm = calculateViewMetrics();
    Matrix4 projection = Matrix4::ortho(-vm.width, vm.width, -vm.height, vm.height, -100.0f, 100.0f);
    Matrix4 view = Matrix4::identity();

    int count = static_cast<int>(indices.size());
    int centerTile = static_cast<int>(std::floor(panOffsetX / MAP_WIDTH));

    for (int tileOffset = -1; tileOffset <= 1; ++tileOffset) {
        int tileIndex = centerTile + tileOffset;
        float tileX = tileIndex * MAP_WIDTH - panOffsetX;

        Matrix4 tileTransform = Matrix4::translate({tileX, -panOffsetY, 0.0f});
        Matrix4 modelTransform = Matrix4::identity();
        Matrix4 mvp = projection * view * tileTransform * modelTransform;

        if (shader) {
            shader->bind();
            shader->setMat4("u_Transform", mvp.data());
            
            vao->bind();
            glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
            vao->unbind();
            
            borderVao->bind();
            glDrawElements(GL_LINES, static_cast<GLsizei>(borderIndices.size()), GL_UNSIGNED_INT, nullptr);
            borderVao->unbind();
            
            shader->unbind();
        }
    }
}

void MapLayer::setCountryColor(const std::string& isoCode, float r, float g, float b, float a) {
    if (countries.find(isoCode) == countries.end()) return;

    const auto& meta = countries[isoCode];
    for (size_t vIdx : meta.globalVertexIndices) {
        vertices[vIdx].color = {r, g, b, a};
    }
    needsBufferUpdate = true;
}

void MapLayer::loadAndTriangulate(const std::string& path) {
    using json = nlohmann::json;
    std::ifstream f(path);
    if(!f.is_open()) { printf("Failed to load Map from %s\n", path.c_str()); return; }

    json data;
    try { f >> data; } catch(...) { return; }

    vertices.clear();
    indices.clear();
    borderVertices.clear();
    borderIndices.clear();

    for (const auto& feature : data["features"]) {
        std::string id = feature["properties"].value("adm0_a3", "UNK");
        CountryMetadata& meta = countries[id];
        meta.name = feature["properties"].value("name", "Unknown");

        auto processPolygon = [&](const std::vector<std::vector<Point>>& rings) {
            std::vector<uint32_t> localIndices = mapbox::earcut<uint32_t>(rings);
            size_t baseIndex = vertices.size();

            for (const auto& ring : rings) {
                for (const auto& p : ring) {
                    auto [x, y] = latLonToWorld(p.x, p.y);
                    MapVertex v;
                    v.position = {x, y, 0.0f};
                    v.normal = {0.0f, 0.0f, 1.0f};
                    v.color = {0.5f, 0.5f, 0.5f, 1.0f}; 
                    
                    meta.globalVertexIndices.push_back(vertices.size());
                    meta.updateBounds(x, y);
                    vertices.push_back(v);
                }
            }
            for (uint32_t idx : localIndices) indices.push_back(static_cast<uint32_t>(baseIndex + idx));
            meta.rawPolygons.push_back(rings[0]);
  
            for (const auto& ring : rings) {
                size_t borderBaseIdx = borderVertices.size();
                for (size_t i = 0; i < ring.size(); ++i) {
                    auto [x, y] = latLonToWorld(ring[i].x, ring[i].y);
                    MapVertex v;
                    v.position = {x, y, 0.01f}; 
                    v.normal   = {0.0f, 0.0f, 1.0f};
                    v.color    = BORDER_COLOR;
                    borderVertices.push_back(v);
                    if (i > 0) {
                        borderIndices.push_back(static_cast<uint32_t>(borderBaseIdx + i - 1));
                        borderIndices.push_back(static_cast<uint32_t>(borderBaseIdx + i));
                    }
                }
            }
        };

        std::string type = feature["geometry"]["type"];
        auto& coords = feature["geometry"]["coordinates"];

        if (type == "Polygon") {
            std::vector<std::vector<Point>> rings;
            for (auto& ringJson : coords) {
                std::vector<Point> ring;
                for (auto& c : ringJson) ring.push_back({(double)c[0], (double)c[1]});
                rings.push_back(ring);
            }
            processPolygon(rings);
        }
        else if (type == "MultiPolygon") {
            for (auto& polyJson : coords) {
                std::vector<std::vector<Point>> rings;
                for (auto& ringJson : polyJson) {
                    std::vector<Point> ring;
                    for (auto& c : ringJson) ring.push_back({(double)c[0], (double)c[1]});
                    rings.push_back(ring);
                }
                processPolygon(rings);
            }
        }
    }
}

void MapLayer::uploadBuffers() {
    vao->bind();
    vbo->bind();
    vbo->data(vertices.size() * sizeof(MapVertex), vertices.data(), GL_DYNAMIC_DRAW);
    ebo->bind();
    ebo->data(indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);
    vao->unbind();

    borderVao->bind();
    borderVbo->bind();
    borderVbo->data(borderVertices.size() * sizeof(MapVertex), borderVertices.data(), GL_STATIC_DRAW);
    borderEbo->bind();
    borderEbo->data(borderIndices.size() * sizeof(uint32_t), borderIndices.data(), GL_STATIC_DRAW);
    borderVao->unbind();

    needsBufferUpdate = false;
}

void MapLayer::handleMousePicking() {
    auto mx = app->getCursorX();
    auto my = app->getCursorY();
    ViewMetrics vm = calculateViewMetrics();
    auto [worldX, worldY] = screenToWorld(mx, my, vm);

    float wrappedX = worldX;
    while (wrappedX < -MAP_HALF_WIDTH) wrappedX += MAP_WIDTH;
    while (wrappedX > MAP_HALF_WIDTH) wrappedX -= MAP_WIDTH;

    for (auto& [id, meta] : countries) {
        if (!meta.contains(wrappedX, worldY)) continue;

        bool inside = false;
        for (const auto& poly : meta.rawPolygons) {
            size_t j = poly.size() - 1;
            for (size_t i = 0; i < poly.size(); i++) {
                auto [polyXi, polyYi] = latLonToWorld(poly[i].x, poly[i].y);
                auto [polyXj, polyYj] = latLonToWorld(poly[j].x, poly[j].y);
                if (((polyYi > worldY) != (polyYj > worldY)) &&
                    (wrappedX < (polyXj - polyXi) * (worldY - polyYi) / (polyYj - polyYi) + polyXi)) {
                    inside = !inside;
                }
                j = i;
            }
            if (inside) break;
        }

        if (inside) {
            if (selectedCountries.find(id) != selectedCountries.end()) {
                deselectCountry(id);
            } else {
                selectCountry(id);
            }
            return;
        }
    }
}

std::string MapLayer::getCountryAtCursor() const {
    auto mx = app->getCursorX();
    auto my = app->getCursorY();
    ViewMetrics vm = calculateViewMetrics();
    auto [worldX, worldY] = screenToWorld(mx, my, vm);

    float wrappedX = worldX;
    while (wrappedX < -MAP_HALF_WIDTH) wrappedX += MAP_WIDTH;
    while (wrappedX > MAP_HALF_WIDTH) wrappedX -= MAP_WIDTH;

    for (const auto& [id, meta] : countries) {
        if (!meta.contains(wrappedX, worldY)) continue;
        bool inside = false;
        for (const auto& poly : meta.rawPolygons) {
            size_t j = poly.size() - 1;
            for (size_t i = 0; i < poly.size(); i++) {
                auto [polyXi, polyYi] = latLonToWorld(poly[i].x, poly[i].y);
                auto [polyXj, polyYj] = latLonToWorld(poly[j].x, poly[j].y);
                if (((polyYi > worldY) != (polyYj > worldY)) &&
                    (wrappedX < (polyXj - polyXi) * (worldY - polyYi) / (polyYj - polyYi) + polyXi)) {
                    inside = !inside;
                }
                j = i;
            }
            if (inside) break;
        }
        if (inside) return id;
    }
    return "";
}

void MapLayer::setupCallbacks() {
    app->getWindowEvents().onMouseClick = [this](int button, int action, int) {
        if (ImGui::GetIO().WantCaptureMouse) return;

        // Selction on left-click, drag logic
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            Camera& cam = app->getCamera();
            if (action == GLFW_PRESS) {
                cam.dragging = true;
                cam.lastMouseX = app->getCursorX();
                cam.lastMouseY = app->getCursorY();
                mousePressX = cam.lastMouseX;
                mousePressY = cam.lastMouseY;
            } else if (action == GLFW_RELEASE) {
                cam.dragging = false;
                double dx = app->getCursorX() - mousePressX;
                double dy = app->getCursorY() - mousePressY;
                if ((dx * dx + dy * dy) < 25.0 && selectionEnabled) {
                    handleMousePicking();
                }
            }

            return;
        }
    };

    app->getWindowEvents().onScroll = [this](double, double yoffset) {
        Camera& cam = app->getCamera();
    

        ViewMetrics oldVm = calculateViewMetrics();
        auto [worldXBefore, worldYBefore] = screenToWorld(app->getCursorX(), app->getCursorY(), oldVm);
        float oldDistance = cam.distance;
        cam.zoom(static_cast<float>(yoffset));
        
        ViewMetrics newVm = calculateViewMetrics();
        if (newVm.height < 1.5f) {
            cam.distance = oldDistance;
            return;
        }

        // Calculate world position after zoom
        auto [worldXAfter, worldYAfter] = screenToWorld(app->getCursorX(), app->getCursorY(), newVm);
        panOffsetX += (worldXBefore - worldXAfter);
        panOffsetY += (worldYBefore - worldYAfter);
        constrainPanY(newVm.height);
    };

    app->getWindowEvents().onMouseMove = [this](double x, double y) {
        Camera& cam = app->getCamera();
        if (cam.dragging) {
            ViewMetrics vm = calculateViewMetrics();
            double dx = x - cam.lastMouseX;
            double dy = y - cam.lastMouseY;
            float worldDx = (float)(dx / app->getWidth()) * (2.0f * vm.width);
            float worldDy = (float)(dy / app->getHeight()) * (2.0f * vm.height);
            panOffsetX -= worldDx;
            panOffsetY += worldDy;
            constrainPanY(vm.height);
            cam.lastMouseX = x;
            cam.lastMouseY = y;
        }
    };
}
#include "MapLayer.hpp"
#include "imgui.h"

const std::array<MapVertex::Attribute, 3> MapVertex::attributes = {{
    {0, 3, GL_FLOAT, offsetof(MapVertex, position)},
    {1, 3, GL_FLOAT, offsetof(MapVertex, normal)},
    {2, 4, GL_FLOAT, offsetof(MapVertex, color)}
}};

void MapLayer::onAttach() {
    vao->create();
    vbo->create();
    ebo->create();

    loadAndTriangulate(mapFilePath);

    uploadBuffers();

    vao->bind();
    vao->setLayout<MapVertex>();
    vao->unbind();

    for (auto& [id, meta] : countries) {
        setCountryColor(id, defaultColor.x, defaultColor.y, defaultColor.z, defaultColor.w);
    }

    uploadBuffers();

    Camera& cam = app->getCamera();
    cam.distance = 15.0f;
    cam.pitch = 0.0f;
    cam.yaw = 0.0f;

    setupCallbacks();
}

void MapLayer::onUpdate(float dt) {
    if (needsBufferUpdate) {
        uploadBuffers();
    }
}

void MapLayer::onRender(Renderer& renderer) {
    Camera& cam = app->getCamera();

    float aspect = float(app->getWidth()) / float(app->getHeight());
    float zoom = cam.distance / 15.0f;
    float orthoHeight = 5.0f * zoom;
    float orthoWidth = orthoHeight * aspect;

    Matrix4 projection = Matrix4::ortho(-orthoWidth, orthoWidth, -orthoHeight, orthoHeight, -100.0f, 100.0f);
    Matrix4 view = Matrix4::identity();

    *indexCount = static_cast<int>(indices.size());

    // Render multiple instances of the map for infinite tiling along x-axis
    // Determine which instances need to be rendered based on camera position
    int centerTile = static_cast<int>(std::floor(panOffsetX / MAP_WIDTH));

    // Render 3 tiles to ensure seamless coverage (left, center, right)
    for (int tileOffset = -1; tileOffset <= 1; ++tileOffset) {
        int tileIndex = centerTile + tileOffset;
        float tileX = tileIndex * MAP_WIDTH - panOffsetX;

        Matrix4 tileTransform = Matrix4::translate({tileX, -panOffsetY, 0.0f});
        Matrix4 mvp = projection * view * tileTransform * modelTransform;

        renderer.submit(vao, indexCount, shader, mvp);
    }
}

void MapLayer::setCountryColor(const std::string& isoCode, float r, float g, float b, float a) {
    if (countries.find(isoCode) == countries.end()) {
        printf("Country not found: %s\n", isoCode.c_str());
        return;
    }

    const auto& meta = countries[isoCode];
    for (size_t vIdx : meta.globalVertexIndices) {
        vertices[vIdx].color = {r, g, b, a};
    }

    needsBufferUpdate = true;
}

void MapLayer::loadAndTriangulate(const std::string& path) {
    using json = nlohmann::json;
    std::ifstream f(path);
    if(!f.is_open()) { printf("Failed to load Map!\n"); return; }

    json data = json::parse(f);

    for (const auto& feature : data["features"]) {
        std::string id = feature["properties"].value("adm0_a3", "UNK");

        CountryMetadata& meta = countries[id];
        meta.name = feature["properties"].value("name", "Unknown");

        auto processPolygon = [&](const std::vector<std::vector<Point>>& rings) {

            std::vector<uint32_t> localIndices = mapbox::earcut<uint32_t>(rings);

            size_t baseIndex = vertices.size();

            for (const auto& ring : rings) {
                for (const auto& p : ring) {
                    // Project Lat/Lon to World Space (e.g., -10 to 10)
                    float x = (float)(p[0] / 180.0 * 10.0);
                    float y = (float)(p[1] / 90.0 * 5.0); // Equirectangular Projection

                    MapVertex v;
                    v.position = {x, y, 0.0f};
                    v.normal = {0.0f, 0.0f, 1.0f};
                    v.color = {0.5f, 0.5f, 0.5f, 1.0f};

                    meta.globalVertexIndices.push_back(vertices.size());
                    meta.updateBounds(x, y);
                    vertices.push_back(v);
                }
            }

            for (uint32_t idx : localIndices) {
                indices.push_back(static_cast<uint32_t>(baseIndex + idx));
            }

            meta.rawPolygons.push_back(rings[0]);
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

    needsBufferUpdate = false;
}

void MapLayer::handleMousePicking() {
    auto mx = app->getCursorX();
    auto my = app->getCursorY();
    int winW = app->getWidth();
    int winH = app->getHeight();

    // Convert mouse to world coordinates using orthographic projection
    Camera& cam = app->getCamera();
    float aspect = float(winW) / float(winH);

    // Calculate the same ortho bounds as in rendering
    float zoom = cam.distance / 15.0f;
    float orthoHeight = 5.0f * zoom;
    float orthoWidth = orthoHeight * aspect;

    // Convert mouse position to NDC
    float ndcX = (2.0f * mx / winW) - 1.0f;
    float ndcY = 1.0f - (2.0f * my / winH);

    // Convert NDC to world space (accounting for pan offset)
    float worldX = ndcX * orthoWidth + panOffsetX;
    float worldY = ndcY * orthoHeight + panOffsetY;

    // Wrap world X to the base map coordinates (-10 to 10)
    float wrappedX = worldX;
    while (wrappedX < -10.0f) wrappedX += MAP_WIDTH;
    while (wrappedX > 10.0f) wrappedX -= MAP_WIDTH;

    int testedCount = 0;
    for (auto& [id, meta] : countries) {

        if (!meta.contains(wrappedX, worldY)) continue;
        testedCount++;

        // Raycast.
        bool inside = false;
        for (const auto& poly : meta.rawPolygons) {
            size_t j = poly.size() - 1;
            for (size_t i = 0; i < poly.size(); i++) {
                float polyXi = (float)(poly[i][0] / 180.0 * 10.0);
                float polyYi = (float)(poly[i][1] / 90.0 * 5.0);
                float polyXj = (float)(poly[j][0] / 180.0 * 10.0);
                float polyYj = (float)(poly[j][1] / 90.0 * 5.0);

                if (((polyYi > worldY) != (polyYj > worldY)) &&
                    (wrappedX < (polyXj - polyXi) * (worldY - polyYi) / (polyYj - polyYi) + polyXi)) {
                    inside = !inside;
                }
                j = i;
            }
            if (inside) break;
        }

        if (inside) {

            // Toggle selection for this country
            if (selectedCountries.find(id) != selectedCountries.end()) {
                // Already selected - deselect it
                selectedCountries.erase(id);
                setCountryColor(id, defaultColor.x, defaultColor.y, defaultColor.z, defaultColor.w);
            } else {
                // Not selected - select it
                selectedCountries.insert(id);
                setCountryColor(id, selectedColor.x, selectedColor.y, selectedColor.z, selectedColor.w);
            }

            return;
        }
    }
}

void MapLayer::setupCallbacks() {
    app->getWindowEvents().onMouseClick = [this](int button, int action, int mods) {

        if (ImGui::GetIO().WantCaptureMouse) {
            printf("Imgui captured mouse click\n");
            return;
        }

        if (button != GLFW_MOUSE_BUTTON_LEFT) return;

        Camera& cam = app->getCamera();
        if (action == GLFW_PRESS) {
            cam.dragging = true;
            cam.lastMouseX = app->getCursorX();
            cam.lastMouseY = app->getCursorY();

            // Store initial press position
            mousePressX = cam.lastMouseX;
            mousePressY = cam.lastMouseY;
        } else if (action == GLFW_RELEASE) {
            // Check if mouse moved significantly from initial press position
            double currentX = app->getCursorX();
            double currentY = app->getCursorY();
            double dx = currentX - mousePressX;
            double dy = currentY - mousePressY;
            double distanceSquared = dx * dx + dy * dy;

            cam.dragging = false;

            // Only handle picking if mouse didn't move much (threshold: 5 pixels)
            if (distanceSquared < 25.0) {
                handleMousePicking();
            }
        }
    };

    app->getWindowEvents().onScroll = [this](double xoffset, double yoffset) {
        Camera& cam = app->getCamera();

        // Get mouse position in world space BEFORE zoom
        auto mx = app->getCursorX();
        auto my = app->getCursorY();
        int winW = app->getWidth();
        int winH = app->getHeight();

        float aspect = float(winW) / float(winH);
        float oldZoom = cam.distance / 15.0f;
        float oldOrthoHeight = 5.0f * oldZoom;
        float oldOrthoWidth = oldOrthoHeight * aspect;

        // Mouse position in NDC
        float ndcX = (2.0f * mx / winW) - 1.0f;
        float ndcY = 1.0f - (2.0f * my / winH);

        // World position before zoom (relative to current pan offset)
        float worldXBefore = ndcX * oldOrthoWidth + panOffsetX;
        float worldYBefore = ndcY * oldOrthoHeight + panOffsetY;

        // Store old distance to potentially revert
        float oldDistance = cam.distance;

        // Apply zoom
        cam.zoom(float(yoffset));

        // Get mouse position in world space AFTER zoom
        float newZoom = cam.distance / 15.0f;
        float newOrthoHeight = 5.0f * newZoom;
        float newOrthoWidth = newOrthoHeight * aspect;

        // Check if the new zoom level would exceed vertical bounds (map height is 10 units: -5 to 5)
        if (newOrthoHeight >= 5.0f) {
            // Revert zoom - viewport would be too large
            cam.distance = oldDistance;
            return;
        }

        float worldXAfter = ndcX * newOrthoWidth + panOffsetX;
        float worldYAfter = ndcY * newOrthoHeight + panOffsetY;

        // Adjust pan offset to keep the mouse cursor over the same world position
        panOffsetX += (worldXBefore - worldXAfter);
        panOffsetY += (worldYBefore - worldYAfter);

        // Clamp vertical panning to map bounds after zoom
        float minY = -5.0f + newOrthoHeight;
        float maxY = 5.0f - newOrthoHeight;

        if (panOffsetY < minY) panOffsetY = minY;
        if (panOffsetY > maxY) panOffsetY = maxY;
    };

    app->getWindowEvents().onMouseMove = [this](double x, double y) {
        Camera& cam = app->getCamera();
        if (cam.dragging) {
            // Calculate mouse delta
            double dx = x - cam.lastMouseX;
            double dy = y - cam.lastMouseY;

            cam.lastMouseX = x;
            cam.lastMouseY = y;

            // Convert pixel delta to world space delta
            float aspect = float(app->getWidth()) / float(app->getHeight());
            float zoom = cam.distance / 15.0f;
            float orthoHeight = 5.0f * zoom;
            float orthoWidth = orthoHeight * aspect;

            // Update pan offsets (inverted for natural dragging)
            panOffsetX -= (float)(dx / app->getWidth() * 2.0 * orthoWidth);
            panOffsetY += (float)(dy / app->getHeight() * 2.0 * orthoHeight);

            // Clamp vertical panning to map bounds (-5 to 5 in world space)
            // Account for the current view height to prevent going past edges
            float minY = -5.0f + orthoHeight;
            float maxY = 5.0f - orthoHeight;

            if (panOffsetY < minY) panOffsetY = minY;
            if (panOffsetY > maxY) panOffsetY = maxY;
        }
    };
}


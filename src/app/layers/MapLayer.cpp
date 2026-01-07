#include "MapLayer.hpp"
#include "imgui.h"

const std::array<MapVertex::Attribute, 3> MapVertex::attributes = {{
    {0, 3, GL_FLOAT, offsetof(MapVertex, position)},
    {1, 3, GL_FLOAT, offsetof(MapVertex, normal)},
    {2, 4, GL_FLOAT, offsetof(MapVertex, color)}
}};

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
    cam.distance = BASE_CAM_DIST;
    cam.pitch = 0.0f;
    cam.yaw = 0.0f;

    setupCallbacks();
}

void MapLayer::onUpdate(float) {
    if (needsBufferUpdate) {
        uploadBuffers();
    }
}

void MapLayer::onRender(Renderer& renderer) {
    ViewMetrics vm = calculateViewMetrics();

    Matrix4 projection = Matrix4::ortho(-vm.width, vm.width, -vm.height, vm.height, -100.0f, 100.0f);
    Matrix4 view = Matrix4::identity();

    *indexCount = static_cast<int>(indices.size());

    int centerTile = static_cast<int>(std::floor(panOffsetX / MAP_WIDTH));

    // Render 3 tiles for infinite scrolling
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
                    auto [x, y] = latLonToWorld(p[0], p[1]);

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

    ViewMetrics vm = calculateViewMetrics();
    auto [worldX, worldY] = screenToWorld(mx, my, vm);

    float wrappedX = worldX;
    while (wrappedX < -MAP_HALF_WIDTH) wrappedX += MAP_WIDTH;
    while (wrappedX > MAP_HALF_WIDTH) wrappedX -= MAP_WIDTH;

    int testedCount = 0;
    for (auto& [id, meta] : countries) {
        // Fast AABB check
        if (!meta.contains(wrappedX, worldY)) continue;
        testedCount++;

        bool inside = false;
        for (const auto& poly : meta.rawPolygons) {
            size_t j = poly.size() - 1;
            for (size_t i = 0; i < poly.size(); i++) {
                auto [polyXi, polyYi] = latLonToWorld(poly[i][0], poly[i][1]);
                auto [polyXj, polyYj] = latLonToWorld(poly[j][0], poly[j][1]);

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
                selectedCountries.insert(id);
                setCountryColor(id, selectedColor.x, selectedColor.y, selectedColor.z, selectedColor.w);
            }
            return;
        }
    }
}

void MapLayer::setupCallbacks() {
    // Mouse click handling for selection.
    app->getWindowEvents().onMouseClick = [this](int button, int action, int) {
        if (ImGui::GetIO().WantCaptureMouse) return;
        if (button != GLFW_MOUSE_BUTTON_LEFT) return;

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
            if ((dx * dx + dy * dy) < 25.0) {
                handleMousePicking();
            }
        }
    };

    // Zooming in and out.
    app->getWindowEvents().onScroll = [this](double, double yoffset) {
        Camera& cam = app->getCamera();

        ViewMetrics oldVm = calculateViewMetrics();
        auto [worldXBefore, worldYBefore] = screenToWorld(app->getCursorX(), app->getCursorY(), oldVm);

        float oldDistance = cam.distance;
        cam.zoom(float(yoffset));

        ViewMetrics newVm = calculateViewMetrics();
        if (newVm.height >= MAP_HALF_HEIGHT) {
            cam.distance = oldDistance; // Revert
            return;
        }

        auto [worldXAfter, worldYAfter] = screenToWorld(app->getCursorX(), app->getCursorY(), newVm);

        panOffsetX += (worldXBefore - worldXAfter);
        panOffsetY += (worldYBefore - worldYAfter);

        constrainPanY(newVm.height);
    };

    // Dragging left and right.
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

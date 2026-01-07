#pragma once

#include "Layer.hpp"
#include "util/DataHandler.hpp"
#include "util/GraphRenderer.hpp"
#include <memory>
#include <string>

class MapLayer;

class UILayer : public Layer {
public:
    explicit UILayer(MapLayer* mapLayer);
    virtual ~UILayer() = default;

    void onUpdate(float deltaTime) override;

private:
    // UI Setup Helpers
    void setupDockspace();
    void renderSelectionList();
    void renderFPSDisplay() const;
    void addSeparatorText(const std::string text) const;
    void renderLoadingScreen();

    // References and Managers
    MapLayer* mapLayer;
    std::unique_ptr<DataHandler> dataManager;
    bool dataInitialized = false;
    bool loadingScreenShown = false;

    // Graph Specifications (Configuration)
    GraphSpec radarSpec;
    GraphSpec splomSpec;
    GraphSpec barSpec;
    GraphSpec scatterSpec;

    // Constants for styling
    static constexpr float SEPARATOR_TEXT_SCALE = 1.5f;
    static constexpr float DEFAULT_TEXT_SCALE = 1.0f;
};

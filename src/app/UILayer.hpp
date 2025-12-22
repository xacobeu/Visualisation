#pragma once

#include "Layer.hpp"

class MapLayer; // Forward declaration

class UILayer : public Layer {

public:
    UILayer() = default;
    UILayer(MapLayer* mapLayer) : mapLayer(mapLayer) {}

    void onUpdate(float deltaTime) override;

private:
    MapLayer* mapLayer = nullptr;

    void setupDockspace();

};

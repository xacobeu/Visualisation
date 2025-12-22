#include "Application.hpp"
#include "GlobeLayer.hpp"
#include "UILayer.hpp"
#include "MapLayer.hpp"

int main() {
    Application app(800, 600, "Visualisation");
    // app.push(std::make_unique<GlobeLayer>());  // Disabled - showing map instead

    auto mapLayer = std::make_unique<MapLayer>();
    MapLayer* mapLayerPtr = mapLayer.get(); // Get raw pointer before moving
    app.push(std::move(mapLayer));

    app.push(std::make_unique<UILayer>(mapLayerPtr));
    app.run();
}

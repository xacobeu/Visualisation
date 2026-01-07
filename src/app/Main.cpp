#include "Application.hpp"
#include "layers/UILayer.hpp"
#include "layers/MapLayer.hpp"

int main() {

    Application app{{ .width = 800, .height = 600, .title = "Visualisation" }};

    app.push(std::make_unique<MapLayer>());
    app.push(std::make_unique<UILayer>(app.getLayer<MapLayer>()));
    app.run();
}

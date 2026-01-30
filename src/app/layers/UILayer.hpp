#pragma once

#include "Layer.hpp"
#include "util/DataHandler.hpp"
#include "util/GraphRenderer.hpp"
#include <memory>
#include <string>
#include <vector>

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
    void renderChoroplethControls();
    void addSeparatorText(const std::string text) const;
    void renderLoadingScreen();
    void renderSearchBar();
    void renderGraphFilterSection(const std::vector<std::string>& countryNames);
    void renderCategoricalFilterSection(const std::vector<std::string>& countryNames);

    // References and Managers
    MapLayer* mapLayer;
    std::unique_ptr<DataHandler> dataManager;
    bool dataInitialized = false;
    bool loadingScreenShown = false;
    
    // Choropleth state
    int selectedChoroplethColumn = -1;
    bool currentIsDiverging = false;
    float currentMinVal = 0.0f;
    float currentMaxVal = 0.0f;

    std::vector<std::string> availableColumns;
    int currentMapTab = 0;  // 0 = Selection, 1 = Choropleth

    // Search Bar State
    char searchBuffer[128] = ""; // <--- Buffer for the input text
    std::vector<std::string> cachedCountryNames;
    
    // Graph filter state (numeric)
    struct NumericFilter {
        int columnIndex = 0;
        int lastColumnIndex = -1;
        float minValue = 0.0f;
        float maxValue = 0.0f;
        std::string unit;
        float dataMin = 0.0f;  // Min value in dataset
        float dataMax = 0.0f;  // Max value in dataset
    };
    bool graphFilterEnabled = false;
    std::vector<NumericFilter> numericFilters;
    
    // Categorical filter state
    struct CategoricalFilter {
        int columnIndex = 0;
        std::vector<std::string> values;        // All unique values for this column
        std::vector<bool> selected;             // Which values are selected
    };
    bool categoricalFilterEnabled = false;
    std::vector<CategoricalFilter> categoricalFilters;

    // Treemap state
    int selectedTreemapMetric = 0;
    
    // Custom TreeMap Builder state
    std::vector<bool> selectedAttributes;
    std::vector<GraphSpec> savedCustomGraphs;
    void renderCustomTreeMapBuilder(const std::string& hoverCountry);

    // Graph Specifications (Configuration)
    GraphSpec radarSpec;
    GraphSpec splomSpec;
    GraphSpec barSpec;
    GraphSpec scatterSpec;
    GraphSpec treemapSpec; 

    // Constants for styling
    static constexpr float SEPARATOR_TEXT_SCALE = 1.5f;
    static constexpr float DEFAULT_TEXT_SCALE = 1.0f;
    
};

#include "UILayer.hpp"
#include "MapLayer.hpp"
#include "../util/GraphRenderer.hpp"

#include <implot.h>
#include <imgui.h>

#include <algorithm>
#include <cstring> // For memset, strncpy
#include <iostream>
#include <unordered_set>

UILayer::UILayer(MapLayer* mapLayer) : mapLayer(mapLayer) {
    dataManager = std::make_unique<DataHandler>();

    // Radar Graph Spec
    radarSpec.type = GraphType::Radar;
    radarSpec.title = "##RadarChart";
    radarSpec.xLabel = "Metrics";
    radarSpec.yLabel = "Values";
    radarSpec.series.columns = {
        "Real_GDP_PPP_billion_USD",
        "Real_GDP_per_Capita_USD",
        "Budget_billion_USD",
        "Budget_Surplus_billion_USD",
        "Exports_billion_USD",
        "Imports_billion_USD",
        "Exchange_Rate_per_USD",
    };
    radarSpec.series.labels = {
        "GDP (PPP) (billion USD)",
        "GDP per Capita (USD)",
        "Budget (billion USD)",
        "Budget Surplus (billion USD)",
        "Exports (billion USD)",
        "Imports (billion USD)",
        "Exchange Rate (per USD)"
    };

    // SPLOM Graph Spec
    splomSpec.type = GraphType::SPLOM;
    splomSpec.title = "##SPLOM";
    splomSpec.xLabel = "Indicators";
    splomSpec.yLabel = "Indicators";
    splomSpec.series.columns = {
        "Real_GDP_per_Capita_USD",
        "Unemployment_Rate_percent",
        "Budget_Deficit_percent_of_GDP",
        "Public_Debt_percent_of_GDP"
    };
    splomSpec.series.labels = {
        "GDP per Capita",
        "Unemployment Rate",
        "Budget Deficit",
        "Public Debt"
    };

    // Treemap Spec
    treemapSpec.type = GraphType::TreeMap;
    treemapSpec.title = "##Treemap";
    treemapSpec.xLabel = "";
    treemapSpec.yLabel = "";
    treemapSpec.series.columns = { "Real_GDP_PPP_billion_USD" };
    treemapSpec.series.labels = { "Real GDP (PPP)" };
}

void UILayer::onUpdate(float) {
    // Show loading screen first
    if (!dataInitialized && !loadingScreenShown) {
        renderLoadingScreen();
        loadingScreenShown = true;
        return;
    }

    // Lazy initialization of data after loading screen is shown
    if (!dataInitialized) {
        dataManager->init();
        availableColumns = dataManager->getAvailableColumns();
        std::sort(availableColumns.begin(), availableColumns.end());
        
        // Cache names for validation
        cachedCountryNames = dataManager->getAllCountryNames(); 
        std::sort(cachedCountryNames.begin(), cachedCountryNames.end());
        
        dataInitialized = true;
    }

    setupDockspace();

    // --- Side Panel: Map Controls ---
    if (ImGui::Begin("Map Controls")) {
        renderFPSDisplay();
        ImGui::Separator();
        
        // Map mode tabs
        addSeparatorText("Map Mode");
        
        if (ImGui::BeginTabBar("MapModeTabs")) {
            if (ImGui::BeginTabItem("Selection")) {
                if (currentMapTab != 0) {
                    currentMapTab = 0;
                    mapLayer->setSelectionEnabled(true);
                    mapLayer->clearChoropleth();
                    selectedChoroplethColumn = -1;
                }
                ImGui::Spacing();
                ImGui::TextDisabled("Click on countries to select them.");
                ImGui::Separator();
                renderSelectionList();
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Choropleth")) {
                if (currentMapTab != 1) {
                    currentMapTab = 1;
                    mapLayer->setSelectionEnabled(false);
                }
                ImGui::Spacing();
                renderChoroplethControls();
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }
    }
    ImGui::End();

    // --- Determine Highlighted Country ---
    // Priority: Mouse Hover > Search Bar result
    std::string hoverCountry = "";
    std::string searchCountry = "";
    
    // 1. Check Search Bar (Exact Match Logic)
    if (strlen(searchBuffer) > 0) {
         std::string s(searchBuffer);
         // Check if the buffer matches a known country exactly
         if (std::find(cachedCountryNames.begin(), cachedCountryNames.end(), s) != cachedCountryNames.end()) {
             searchCountry = s;
         }
    }

    // 2. Check Mouse Hover (Overrides Search for visual feedback)
    if (!ImGui::GetIO().WantCaptureMouse) {
        std::string hovered = mapLayer->getCountryAtCursor();
        if (!hovered.empty()) {
            hoverCountry = mapLayer->getCountryName(hovered);
        }
    }
    
    // Use hover if present, otherwise use search
    if (!hoverCountry.empty()) {
        // Mouse hover takes priority
        std::string hoverIso = mapLayer->getIsoCodeFromName(hoverCountry);
        mapLayer->setHover(hoverIso);
        mapLayer->setSearchHighlight("");
    } else if (!searchCountry.empty()) {
        // Search bar highlight
        std::string searchIso = mapLayer->getIsoCodeFromName(searchCountry);
        mapLayer->setSearchHighlight(searchIso);
        mapLayer->setHover("");
        hoverCountry = searchCountry; // Use search for graph highlighting
    } else {
        mapLayer->setHover("");
        mapLayer->setSearchHighlight("");
    }
    
    // Update map with brushed selection from graphs
    std::unordered_set<std::string> brushedIsos;
    if (GraphRenderer::s_HasSelection && !GraphRenderer::s_HighlightedPoints.empty()) {
        const auto& selectedIds = mapLayer->getSelectedCountries();
        std::vector<std::string> countryNames;
        for (const auto& id : selectedIds) {
            countryNames.push_back(mapLayer->getCountryName(id));
        }
        
        for (size_t i = 0; i < countryNames.size() && i < GraphRenderer::s_HighlightedPoints.size(); ++i) {
            if (GraphRenderer::s_HighlightedPoints[i]) {
                std::string iso = mapLayer->getIsoCodeFromName(countryNames[i]);
                if (!iso.empty()) {
                    brushedIsos.insert(iso);
                }
            }
        }
    }
    mapLayer->setHighlightedCountries(brushedIsos);

    // --- Main Panel: Graphs ---
    if (ImGui::Begin("Graphs")) {

        // --- SEARCH BAR (Now filters selected countries only) ---
        renderSearchBar();
        ImGui::Separator();

        const auto& selectedIds = mapLayer->getSelectedCountries();

        if (selectedIds.empty()) {
            ImGui::TextDisabled("No countries selected. Click on the map to select countries.");
        } else {
            // Convert IDs to Names and Sort Alphabetically.
            std::vector<std::string> countryNames;
            countryNames.reserve(selectedIds.size());
            for (const auto& id : selectedIds) {
                countryNames.push_back(mapLayer->getCountryName(id));
            }
            std::sort(countryNames.begin(), countryNames.end());

            // SPLOM - keep original spec, add other columns to data only
            std::vector<std::string> splomAllColumns = splomSpec.series.columns;
            for (const auto& col : availableColumns) {
                if (std::find(splomSpec.series.columns.begin(), splomSpec.series.columns.end(), col) == splomSpec.series.columns.end()) {
                    splomAllColumns.push_back(col);
                }
            }
            auto splomData = dataManager->collectSeries(countryNames, splomAllColumns);
            GraphRenderer::Render(splomSpec, countryNames, splomData, hoverCountry);

            ImGui::Separator();

            // Radar - keep original spec, add other columns to data only
            std::vector<std::string> radarAllColumns = radarSpec.series.columns;
            for (const auto& col : availableColumns) {
                if (std::find(radarSpec.series.columns.begin(), radarSpec.series.columns.end(), col) == radarSpec.series.columns.end()) {
                    radarAllColumns.push_back(col);
                }
            }
            auto radarData = dataManager->collectSeries(countryNames, radarAllColumns);
            GraphRenderer::Render(radarSpec, countryNames, radarData, hoverCountry);

            ImGui::Separator();

            // Treemap
            const char* treemapMetrics[] = {
                "GDP (PPP)",
                "GDP per Capita",
                "Budget",
                "Exports",
                "Imports",
                "Population"
            };
            const char* treemapColumns[] = {
                "Real_GDP_PPP_billion_USD",
                "Real_GDP_per_Capita_USD",
                "Budget_billion_USD",
                "Exports_billion_USD",
                "Imports_billion_USD",
                "Total_Population"
            };
            
            // Center the Treemap Metric dropdown
            float comboWidth = 200.0f;
            float availWidth = ImGui::GetContentRegionAvail().x;
            float offset = (availWidth - comboWidth) * 0.5f;
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (offset > 0 ? offset : 0));
            ImGui::SetNextItemWidth(comboWidth);
            if (ImGui::Combo("Treemap Metric", &selectedTreemapMetric, treemapMetrics, IM_ARRAYSIZE(treemapMetrics))) {
                // Update treemap spec when selection changes
                treemapSpec.series.columns = { treemapColumns[selectedTreemapMetric] };
                treemapSpec.series.labels = { treemapMetrics[selectedTreemapMetric] };
            }
            
            auto treemapData = dataManager->collectSeries(countryNames, treemapSpec.series.columns);
            
            // Build continent map for treemap
            std::unordered_map<std::string, std::string> continentMap;
            for (const auto& country : countryNames) {
                continentMap[country] = dataManager->getContinent(country);
            }
            
            GraphRenderer::Render(treemapSpec, countryNames, treemapData, hoverCountry, continentMap);
            
            ImGui::Separator();
            
            // Custom TreeMap Builder
            renderCustomTreeMapBuilder(hoverCountry);
        }
    }
    ImGui::End();

    // --- Hover Tooltip for Map ---
    if (!ImGui::GetIO().WantCaptureMouse) {
        std::string hoveredCountry = mapLayer->getCountryAtCursor();
        if (!hoveredCountry.empty()) {
            std::string countryName = mapLayer->getCountryName(hoveredCountry);
            
            ImGui::BeginTooltip();
            ImGui::Text("%s", countryName.c_str());
            
            if (mapLayer->isChoroplethActive()) {
                float value;
                if (mapLayer->getChoroplethValue(hoveredCountry, value)) {
                    std::string metricName = (selectedChoroplethColumn >= 0 && selectedChoroplethColumn < static_cast<int>(availableColumns.size()))
                        ? availableColumns[selectedChoroplethColumn]
                        : "Value";
                    const std::string& unit = mapLayer->getChoroplethUnit();
                    if (!unit.empty()) {
                        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s: %.2f %s", metricName.c_str(), value, unit.c_str());
                    } else {
                        ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s: %.2f", metricName.c_str(), value);
                    }
                } else {
                    ImGui::TextDisabled("No data available");
                }
            }
            ImGui::EndTooltip();
        }
    }
}

void UILayer::renderSearchBar() {
    addSeparatorText("Find in Selection");
    
    // Input box
    ImGui::SetNextItemWidth(-1); // Use full width
    if (ImGui::InputTextWithHint("##Search", "Filter active graphs...", searchBuffer, IM_ARRAYSIZE(searchBuffer))) {
        // Just keeping buffer updated
    }
    
    // Dropdown suggestions - Only search WITHIN selected countries
    if (strlen(searchBuffer) > 0) {
        // Only show popup if we have selected countries
        const auto& selectedIds = mapLayer->getSelectedCountries();
        if (selectedIds.empty()) return;

        if (ImGui::BeginChild("SearchResults", ImVec2(0, 100), true)) {
            std::string query = searchBuffer;
            std::transform(query.begin(), query.end(), query.begin(), ::tolower);
            
            // Build temporary list of NAMES from selected IDs
            std::vector<std::string> activeNames;
            activeNames.reserve(selectedIds.size());
            for(const auto& id : selectedIds) {
                activeNames.push_back(mapLayer->getCountryName(id));
            }
            // Sort for display
            std::sort(activeNames.begin(), activeNames.end());

            bool foundAny = false;
            for (const auto& country : activeNames) {
                std::string countryLower = country;
                std::transform(countryLower.begin(), countryLower.end(), countryLower.begin(), ::tolower);
                
                // Simple substring match
                if (countryLower.find(query) != std::string::npos) {
                    foundAny = true;
                    if (ImGui::Selectable(country.c_str())) {
                        // On click: Set the buffer to the full name. 
                        // The 'onUpdate' loop will pick this up and set 'highlightCountry'
                        memset(searchBuffer, 0, sizeof(searchBuffer));
                        strncpy(searchBuffer, country.c_str(), sizeof(searchBuffer) - 1);
                        searchBuffer[sizeof(searchBuffer) - 1] = '\0';
                    }
                }
            }
            if (!foundAny) {
                ImGui::TextDisabled("No match in current selection");
            }
        }
        ImGui::EndChild();
    }
}

void UILayer::renderChoroplethControls() {
    ImGui::TextDisabled("Color map by data metric:");
    
    const char* previewValue = (selectedChoroplethColumn >= 0 && selectedChoroplethColumn < static_cast<int>(availableColumns.size()))
        ? availableColumns[selectedChoroplethColumn].c_str()
        : "Select metric...";
    
    if (ImGui::BeginCombo("##ChoroplethMetric", previewValue)) {
        for (int i = 0; i < static_cast<int>(availableColumns.size()); ++i) {
            bool isSelected = (selectedChoroplethColumn == i);
            if (ImGui::Selectable(availableColumns[i].c_str(), isSelected)) {
                
                selectedChoroplethColumn = i;
                auto columnData = dataManager->getColumnWithUnitForAllCountries(availableColumns[i]);
                
                currentIsDiverging = columnData.isDiverging;
                
                currentMinVal = std::numeric_limits<float>::max();
                currentMaxVal = std::numeric_limits<float>::lowest();
                
                std::unordered_map<std::string, float> isoData;
                for (const auto& [countryName, value] : columnData.values) {
                    std::string isoCode = mapLayer->getIsoCodeFromName(countryName);
                    if (!isoCode.empty()) {
                        isoData[isoCode] = value;
                        if (value < currentMinVal) currentMinVal = value;
                        if (value > currentMaxVal) currentMaxVal = value;
                    }
                }
                
                mapLayer->applyChoropleth(isoData, columnData.unit, columnData.isDiverging);
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    
    if (mapLayer->isChoroplethActive()) {
        ImGui::Spacing();
        ImGui::TextDisabled("Legend:");
        
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float width = ImGui::GetContentRegionAvail().x;
        float height = 20.0f;
        
        // Resolve Colormap ID
        ImPlotColormap mapId;
        if (currentIsDiverging) {
            // Attempt to find the custom map by name
            mapId = ImPlot::GetColormapIndex("PrGn_Custom");
            // Fallback to RdBu if the custom one wasn't registered yet
            if (mapId == -1) mapId = ImPlotColormap_RdBu; 
        } else {
            mapId = ImPlotColormap_Viridis;
        }

        // Draw Gradient
        int segments = 64;
        float segWidth = width / segments;

        for (int i = 0; i < segments; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(segments - 1);
            ImVec4 c = ImPlot::SampleColormap(t, mapId);
            ImU32 col = ImGui::ColorConvertFloat4ToU32(c);
            
            drawList->AddRectFilled(
                ImVec2(pos.x + i * segWidth, pos.y), 
                ImVec2(pos.x + (i + 1) * segWidth, pos.y + height), 
                col
            );
        }
        ImGui::Dummy(ImVec2(width, height));

        // Draw Labels (Perfectly Aligned)
        
        // Format strings first to calculate exact sizes
        char minBuf[32]; 
        snprintf(minBuf, sizeof(minBuf), "%.1f", currentMinVal);
        
        char maxBuf[32]; 
        snprintf(maxBuf, sizeof(maxBuf), "%.1f", currentMaxVal);

        float startX = ImGui::GetCursorPosX();

        // --- Left Label (Min) ---
        ImGui::Text("%s", minBuf);

        // --- Center Label ("0") ---
        if (currentIsDiverging) {
            const char* midStr = "0";
            float midTextWidth = ImGui::CalcTextSize(midStr).x;
            ImGui::SameLine();
            ImGui::SetCursorPosX(startX + (width * 0.5f) - (midTextWidth * 0.5f));
            ImGui::Text("%s", midStr);
        }

        // --- Right Label (Max) ---
        float maxTextWidth = ImGui::CalcTextSize(maxBuf).x;
        ImGui::SameLine();
        ImGui::SetCursorPosX(startX + width - maxTextWidth);
        ImGui::Text("%s", maxBuf);
    }
}

void UILayer::renderSelectionList() {
    addSeparatorText("Selected Countries");
    
    // Search bar for adding countries
    static char countrySearchBuffer[128] = "";
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##CountrySearch", "Search countries to select...", countrySearchBuffer, IM_ARRAYSIZE(countrySearchBuffer));
    
    // Show search results dropdown
    if (strlen(countrySearchBuffer) > 0) {
        if (ImGui::BeginChild("CountrySearchResults", ImVec2(0, 150), true)) {
            std::string query = countrySearchBuffer;
            std::transform(query.begin(), query.end(), query.begin(), ::tolower);
            
            // Get all available countries
            auto allCountryNames = dataManager->getAllCountryNames();
            std::sort(allCountryNames.begin(), allCountryNames.end());
            
            bool foundAny = false;
            for (const auto& countryName : allCountryNames) {
                std::string countryLower = countryName;
                std::transform(countryLower.begin(), countryLower.end(), countryLower.begin(), ::tolower);
                
                if (countryLower.find(query) != std::string::npos) {
                    foundAny = true;
                    std::string isoCode = mapLayer->getIsoCodeFromName(countryName);
                    if (!isoCode.empty()) {
                        const auto& selected = mapLayer->getSelectedCountries();
                        bool isSelected = selected.find(isoCode) != selected.end();
                        
                        // Show checkbox for selection state
                        if (ImGui::Selectable(countryName.c_str(), isSelected)) {
                            if (isSelected) {
                                mapLayer->deselectCountry(isoCode);
                            } else {
                                mapLayer->selectCountry(isoCode);
                            }
                        }
                    }
                }
            }
            
            if (!foundAny) {
                ImGui::TextDisabled("No countries found");
            }
        }
        ImGui::EndChild();
    }
    
    const auto& selectedCountries = mapLayer->getSelectedCountries();

    if (selectedCountries.empty()) {
        ImGui::TextDisabled("No countries selected (Click on countries to select them)");
    } else {
        if (ImGui::BeginChild("CountryList", ImVec2(0, 200), true))
        {
            std::string toDeselect = "";
            for (const auto& countryId : selectedCountries) {
                std::string countryName = mapLayer->getCountryName(countryId);

                ImGui::PushID(countryId.c_str());
                ImGui::Text("%s", countryName.c_str());

                float buttonWidth = ImGui::CalcTextSize("X").x + ImGui::GetStyle().FramePadding.x * 2.0f;
                ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - buttonWidth);

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

                if (ImGui::SmallButton("X")) toDeselect = countryId;

                ImGui::PopStyleColor(3);
                ImGui::PopID();
            }
            if (!toDeselect.empty()) mapLayer->deselectCountry(toDeselect);
        }
        ImGui::EndChild();

        if (ImGui::Button("Clear All")) mapLayer->clearSelection();
        ImGui::SameLine();
    }

    if (!selectedCountries.empty()) {
        float buttonWidth = ImGui::CalcTextSize("Select All").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - buttonWidth - 7.0f);
    }
    if (ImGui::Button("Select All")) mapLayer->selectAll();
}

void UILayer::renderCustomTreeMapBuilder(const std::string& hoverCountry) {
    const auto& selectedCountries = mapLayer->getSelectedCountries();
    
    // Get country names from selected IDs
    std::vector<std::string> countryNames;
    countryNames.reserve(selectedCountries.size());
    for (const auto& id : selectedCountries) {
        countryNames.push_back(mapLayer->getCountryName(id));
    }
    
    // Filter columns suitable for treemaps (absolute values, not rates/percentages/ratios)
    static std::vector<std::string> treemapCompatibleColumns;
    if (treemapCompatibleColumns.empty() && !availableColumns.empty()) {
        static const std::unordered_set<std::string> excludedColumns = {
            // Exclude rates and percentages
            "Population_Growth_Rate", "Birth_Rate", "Death_Rate", "Net_Migration_Rate",
            "Real_GDP_Growth_Rate_percent", "Unemployment_Rate_percent", "Youth_Unemployment_Rate_percent",
            "Budget_Deficit_percent_of_GDP", "Public_Debt_percent_of_GDP",
            "Population_Below_Poverty_Line_percent", "electricity_access_percent", "Arable_Land_percent",
            // Exclude ratios and per-capita values
            "Exchange_Rate_per_USD", "Real_GDP_per_Capita_USD", "Median_Age",
            // Exclude elevations (can be negative, not meaningful for treemap size)
            "Highest_Elevation", "Lowest_Elevation"
        };
        
        for (const auto& col : availableColumns) {
            if (excludedColumns.find(col) == excludedColumns.end()) {
                treemapCompatibleColumns.push_back(col);
            }
        }
    }
    
    // Render all saved custom treemaps first
    for (size_t graphIdx = 0; graphIdx < savedCustomGraphs.size(); ++graphIdx) {
        // Use unique string ID for each custom treemap to ensure drag-and-drop works independently
        std::string uniqueId = "CustomTreeMap_" + std::to_string(graphIdx);
        ImGui::PushID(uniqueId.c_str());
        
        auto customData = dataManager->collectSeries(countryNames, savedCustomGraphs[graphIdx].series.columns);
        
        // Add delete button
        if (ImGui::Button("Delete TreeMap")) {
            savedCustomGraphs.erase(savedCustomGraphs.begin() + graphIdx);
            ImGui::PopID();
            break;
        }
        ImGui::SameLine();
        ImGui::Text("%s", savedCustomGraphs[graphIdx].title.c_str());
        
        // Build continent map for treemap
        std::unordered_map<std::string, std::string> continentMap;
        for (const auto& country : countryNames) {
            continentMap[country] = dataManager->getContinent(country);
        }
        GraphRenderer::Render(savedCustomGraphs[graphIdx], countryNames, customData, hoverCountry, continentMap);
        
        ImGui::Separator();
        ImGui::PopID();
    }
    
    // Now show the builder menu
    addSeparatorText("Custom TreeMap Builder");
    
    if (selectedCountries.empty()) {
        ImGui::TextDisabled("Select countries to create custom treemaps");
        return;
    }
    
    if (treemapCompatibleColumns.empty()) {
        ImGui::TextDisabled("No treemap-compatible metrics available");
        return;
    }
    
    // Initialize selectedAttributes if needed (based on treemap-compatible columns)
    if (selectedAttributes.size() != treemapCompatibleColumns.size()) {
        selectedAttributes.resize(treemapCompatibleColumns.size(), false);
    }
    
    ImGui::Spacing();
    
    // Metric Selection for TreeMap
    ImGui::Text("Select Metric for TreeMap:");
    
    if (ImGui::BeginChild("AttributeSelection", ImVec2(0, 200), true)) {
        for (size_t i = 0; i < treemapCompatibleColumns.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            bool isSelected = selectedAttributes[i];
            if (ImGui::Checkbox(treemapCompatibleColumns[i].c_str(), &isSelected)) {
                selectedAttributes[i] = isSelected;
            }
            ImGui::PopID();
        }
    }
    ImGui::EndChild();
    
    // Count selected attributes
    int selectedCount = 0;
    for (bool selected : selectedAttributes) {
        if (selected) selectedCount++;
    }
    
    ImGui::Text("Selected: %d metric", selectedCount);
    
    // Show requirements for treemap
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Requirements:");
    ImGui::SameLine();
    ImGui::Text("Exactly 1 metric");
    
    // Check if requirements are met
    bool requirementsMet = (selectedCount == 1);
    std::string errorMsg = "";
    if (!requirementsMet && selectedCount > 0) {
        errorMsg = "TreeMap requires exactly 1 metric";
    }
    
    if (!errorMsg.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", errorMsg.c_str());
    }
    
    // Generate button
    ImGui::Spacing();
    if (!requirementsMet) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Generate TreeMap", ImVec2(200, 0))) {
        if (selectedCount == 1) {
            // Build column list from selected metric
            std::vector<std::string> selectedCols;
            std::vector<std::string> selectedLabels;
            
            for (size_t i = 0; i < treemapCompatibleColumns.size(); ++i) {
                if (selectedAttributes[i]) {
                    selectedCols.push_back(treemapCompatibleColumns[i]);
                    selectedLabels.push_back(treemapCompatibleColumns[i]);
                }
            }
            
            // Create new treemap spec
            GraphSpec newGraph;
            newGraph.type = GraphType::TreeMap;
            newGraph.series.columns = selectedCols;
            newGraph.series.labels = selectedLabels;
            newGraph.title = "Custom TreeMap: " + selectedCols[0];
            newGraph.xLabel = "Countries";
            newGraph.yLabel = "Values";
            
            // Add to saved treemaps
            savedCustomGraphs.push_back(newGraph);
        }
    }
    if (!requirementsMet) {
        ImGui::EndDisabled();
    }
    
    // Clear button
    ImGui::SameLine();
    if (ImGui::Button("Clear Selection", ImVec2(200, 0))) {
        for (size_t i = 0; i < selectedAttributes.size(); ++i) {
            selectedAttributes[i] = false;
        }
    }
    
    // Clear all treemaps button
    if (!savedCustomGraphs.empty()) {
        ImGui::SameLine();
        if (ImGui::Button("Clear All TreeMaps", ImVec2(200, 0))) {
            savedCustomGraphs.clear();
        }
    }
}

void UILayer::setupDockspace() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspace_id = ImGui::GetID("dockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Controls");
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    ImGui::End();
}

void UILayer::renderFPSDisplay() const {
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
}

void UILayer::renderLoadingScreen() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("LoadingScreen", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    ImVec2 windowSize = ImGui::GetWindowSize();
    const char* loadingText = "Loading Data...";
    ImVec2 textSize = ImGui::CalcTextSize(loadingText);

    ImGui::SetCursorPos(ImVec2((windowSize.x - textSize.x) * 0.5f, (windowSize.y - textSize.y) * 0.5f - 20.0f));
    ImGui::Text("%s", loadingText);

    ImGui::SetCursorPos(ImVec2((windowSize.x) * 0.5f - 15.0f, (windowSize.y) * 0.5f + 10.0f));
    static float rotation = 0.0f;
    rotation += 0.05f;
    if (rotation > 6.28f) rotation = 0.0f;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 center = ImVec2(windowSize.x * 0.5f, windowSize.y * 0.5f + 30.0f);
    float radius = 15.0f;

    for (int i = 0; i < 8; i++) {
        float angle = rotation + (i * 3.14159f / 4.0f);
        float alpha = 1.0f - (i / 8.0f);
        ImVec2 pos = ImVec2(center.x + cos(angle) * radius, center.y + sin(angle) * radius);
        draw_list->AddCircleFilled(pos, 3.0f, ImColor(1.0f, 1.0f, 1.0f, alpha));
    }
    ImGui::End();
}

void UILayer::addSeparatorText(const std::string text) const {
    ImGui::SetWindowFontScale(SEPARATOR_TEXT_SCALE);
    ImGui::SeparatorText(text.c_str());
    ImGui::SetWindowFontScale(DEFAULT_TEXT_SCALE);
}
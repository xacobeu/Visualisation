#include "UILayer.hpp"
#include "MapLayer.hpp"

#include <implot.h>
#include <imgui.h>

#include <algorithm>
#include <cstring> // For memset, strncpy
#include <iostream>

UILayer::UILayer(MapLayer* mapLayer) : mapLayer(mapLayer) {
    dataManager = std::make_unique<DataHandler>();

    // Radar Graph Spec
    radarSpec.type = GraphType::Radar;
    radarSpec.title = "";
    radarSpec.xLabel = "Metrics";
    radarSpec.yLabel = "Values";
    radarSpec.series.columns = {
        "telephone_fixed_subscriptions_total", "mobile_cellular_subscriptions_total", "internet_users_total", 
        "broadband_fixed_subscriptions_total", "Total_Population", "Birth_Rate", 
        "Death_Rate", "Median_Age", "Sex_Ratio", "Infant_Mortality_Rate", 
        "Total_Fertility_Rate", "Total_Literacy_Rate", "Male_Literacy_Rate", "Female_Literacy_Rate", 
        "Youth_Unemployment_Rate", "Real_GDP_PPP_billion_USD", "GDP_Official_Exchange_Rate_billion_USD", 
        "Real_GDP_Growth_Rate_percent", "Real_GDP_per_Capita_USD", "Unemployment_Rate_percent", 
        "Youth_Unemployment_Rate_percent", "Budget_billion_USD", "Budget_Surplus_billion_USD", 
        "Budget_Deficit_percent_of_GDP", "Public_Debt_percent_of_GDP", "Exports_billion_USD", 
        "Imports_billion_USD", "Exchange_Rate_per_USD", "Population_Below_Poverty_Line_percent", 
        "electricity_access_percent", "electricity_generating_capacity_kW", "coal_metric_tons", 
        "petroleum_bbl_per_day", "refined_petroleum_products_bbl_per_day", "refined_petroleum_exports_bbl_per_day", 
        "refined_petroleum_imports_bbl_per_day", "natural_gas_cubic_meters", "carbon_dioxide_emissions_Mt", 
        "Area_Total", "Highest_Elevation", "Lowest_Elevation", "Forest_Land", "Other_Land", 
        "Agricultural_Land", "Arable_Land_percent", "Suffrage_Age"
    };
    radarSpec.series.labels = {
        "Fixed Telephone Subscriptions", "Mobile Cellular Subscriptions", "Total Internet Users", 
        "Fixed Broadband Subscriptions", "Total Population", "Birth Rate", 
        "Death Rate", "Median Age", "Sex Ratio", "Infant Mortality Rate", 
        "Total Fertility Rate", "Total Literacy Rate", "Male Literacy Rate", "Female Literacy Rate", 
        "Youth Unemployment Rate", "Real GDP (PPP) $B", "GDP (Official Exchange) $B", 
        "Real GDP Growth Rate %", "Real GDP per Capita $", "Unemployment Rate %", 
        "Youth Unemployment Rate %", "Budget $B", "Budget Surplus $B", 
        "Budget Deficit % of GDP", "Public Debt % of GDP", "Exports $B", 
        "Imports $B", "Exchange Rate per USD", "Population Below Poverty Line %", 
        "Electricity Access %", "Electricity Generating Capacity kW", "Coal (Metric Tons)", 
        "Petroleum (bbl/day)", "Refined Petroleum Products (bbl/day)", "Refined Petroleum Exports (bbl/day)", 
        "Refined Petroleum Imports (bbl/day)", "Natural Gas (cu m)", "CO2 Emissions (Mt)", 
        "Total Area", "Highest Elevation", "Lowest Elevation", "Forest Land", "Other Land", 
        "Agricultural Land", "Arable Land %", "Suffrage Age"
    };

    // SPLOM Graph Spec
    splomSpec.type = GraphType::SPLOM;
    splomSpec.title = "";
    splomSpec.xLabel = "Indicators";
    splomSpec.yLabel = "Indicators";
    splomSpec.series.columns = {
        "telephone_fixed_subscriptions_total", "mobile_cellular_subscriptions_total", "internet_users_total", 
        "broadband_fixed_subscriptions_total", "Total_Population", "Population_Growth_Rate", "Birth_Rate", 
        "Death_Rate", "Net_Migration_Rate", "Median_Age", "Sex_Ratio", "Infant_Mortality_Rate", 
        "Total_Fertility_Rate", "Total_Literacy_Rate", "Male_Literacy_Rate", "Female_Literacy_Rate", 
        "Youth_Unemployment_Rate", "Real_GDP_PPP_billion_USD", "GDP_Official_Exchange_Rate_billion_USD", 
        "Real_GDP_Growth_Rate_percent", "Real_GDP_per_Capita_USD", "Unemployment_Rate_percent", 
        "Youth_Unemployment_Rate_percent", "Budget_billion_USD", "Budget_Surplus_billion_USD", 
        "Budget_Deficit_percent_of_GDP", "Public_Debt_percent_of_GDP", "Exports_billion_USD", 
        "Imports_billion_USD", "Exchange_Rate_per_USD", "Population_Below_Poverty_Line_percent", 
        "electricity_access_percent", "electricity_generating_capacity_kW", "coal_metric_tons", 
        "petroleum_bbl_per_day", "refined_petroleum_products_bbl_per_day", "refined_petroleum_exports_bbl_per_day", 
        "refined_petroleum_imports_bbl_per_day", "natural_gas_cubic_meters", "carbon_dioxide_emissions_Mt", 
        "Area_Total", "Highest_Elevation", "Lowest_Elevation", "Forest_Land", "Other_Land", 
        "Agricultural_Land", "Arable_Land_percent", "Suffrage_Age"
    };
    splomSpec.series.labels = {
        "Fixed Telephone Subscriptions", "Mobile Cellular Subscriptions", "Total Internet Users", 
        "Fixed Broadband Subscriptions", "Total Population", "Population Growth Rate", "Birth Rate", 
        "Death Rate", "Net Migration Rate", "Median Age", "Sex Ratio", "Infant Mortality Rate", 
        "Total Fertility Rate", "Total Literacy Rate", "Male Literacy Rate", "Female Literacy Rate", 
        "Youth Unemployment Rate", "Real GDP (PPP) $B", "GDP (Official Exchange) $B", 
        "Real GDP Growth Rate %", "Real GDP per Capita $", "Unemployment Rate %", 
        "Youth Unemployment Rate %", "Budget $B", "Budget Surplus $B", 
        "Budget Deficit % of GDP", "Public Debt % of GDP", "Exports $B", 
        "Imports $B", "Exchange Rate per USD", "Population Below Poverty Line %", 
        "Electricity Access %", "Electricity Generating Capacity kW", "Coal (Metric Tons)", 
        "Petroleum (bbl/day)", "Refined Petroleum Products (bbl/day)", "Refined Petroleum Exports (bbl/day)", 
        "Refined Petroleum Imports (bbl/day)", "Natural Gas (cu m)", "CO2 Emissions (Mt)", 
        "Total Area", "Highest Elevation", "Lowest Elevation", "Forest Land", "Other Land", 
        "Agricultural Land", "Arable Land %", "Suffrage Age"
    };

    // Treemap Spec
    treemapSpec.type = GraphType::TreeMap;
    treemapSpec.title = "";
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
                
        if (ImGui::BeginTabBar("MapModeTabs")) {
            if (ImGui::BeginTabItem("Selection")) {
                if (currentMapTab != 0) {
                    currentMapTab = 0;
                    mapLayer->setSelectionEnabled(true);
                    mapLayer->clearChoropleth();
                    selectedChoroplethColumn = -1;
                }

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
    
    // 1. Check Search Bar (Exact Match Logic)
    if (strlen(searchBuffer) > 0) {
         std::string s(searchBuffer);
         // Check if the buffer matches a known country exactly
         if (std::find(cachedCountryNames.begin(), cachedCountryNames.end(), s) != cachedCountryNames.end()) {
             hoverCountry = s;
         }
    }

    // 2. Check Mouse Hover (Overrides Search for visual feedback)
    if (!ImGui::GetIO().WantCaptureMouse) {
        std::string hovered = mapLayer->getCountryAtCursor();
        if (!hovered.empty()) {
            hoverCountry = mapLayer->getCountryName(hovered);
        }
    }

    // Push hover to map (uses ISO code)
    if (!hoverCountry.empty()) {
        std::string hoverIso = mapLayer->getIsoCodeFromName(hoverCountry);
        mapLayer->setHover(hoverIso);
    } else {
        mapLayer->setHover("");
    }

    // --- Main Panel: Graphs ---
    if (ImGui::Begin("Graphs")) {

        // --- SEARCH BAR (Now filters selected countries only) ---
        renderSearchBar();
        ImGui::Separator();

        const auto& selectedIds = mapLayer->getSelectedCountries();

        if (selectedIds.empty()) {
            ImGui::TextDisabled("Click on the map to select countries.");
        } else {
            // Convert IDs to Names and Sort Alphabetically.
            std::vector<std::string> countryNames;
            countryNames.reserve(selectedIds.size());
            for (const auto& id : selectedIds) {
                countryNames.push_back(mapLayer->getCountryName(id));
            }
            std::sort(countryNames.begin(), countryNames.end());

            // SPLOM
            addSeparatorText("SPLOM");
            auto splomData = dataManager->collectSeries(countryNames, splomSpec.series.columns);
            GraphRenderer::Render(splomSpec, countryNames, splomData, hoverCountry);

            // Radar
            addSeparatorText("Radar");
            auto radarData = dataManager->collectSeries(countryNames, radarSpec.series.columns);
            GraphRenderer::Render(radarSpec, countryNames, radarData, hoverCountry);

            // Treemap
            addSeparatorText("Treemap");
            const char* treemapMetrics[] = {
                "Fixed Telephone Subscriptions", "Mobile Cellular Subscriptions", "Total Internet Users", 
                "Fixed Broadband Subscriptions", "Total Population", "Population Growth Rate", "Birth Rate", 
                "Death Rate", "Net Migration Rate", "Median Age", "Sex Ratio", "Infant Mortality Rate", 
                "Total Fertility Rate", "Total Literacy Rate", "Male Literacy Rate", "Female Literacy Rate", 
                "Youth Unemployment Rate", "Real GDP (PPP) $B", "GDP (Official Exchange) $B", 
                "Real GDP Growth Rate %", "Real GDP per Capita $", "Unemployment Rate %", 
                "Youth Unemployment Rate %", "Budget $B", "Budget Surplus $B", 
                "Budget Deficit % of GDP", "Public Debt % of GDP", "Exports $B", 
                "Imports $B", "Exchange Rate per USD", "Population Below Poverty Line %", 
                "Electricity Access %", "Electricity Generating Capacity kW", "Coal (Metric Tons)", 
                "Petroleum (bbl/day)", "Refined Petroleum Products (bbl/day)", "Refined Petroleum Exports (bbl/day)", 
                "Refined Petroleum Imports (bbl/day)", "Natural Gas (cu m)", "CO2 Emissions (Mt)", 
                "Total Area", "Highest Elevation", "Lowest Elevation", "Forest Land", "Other Land", 
                "Agricultural Land", "Arable Land %",
            };
            const char* treemapColumns[] = {
                "telephone_fixed_subscriptions_total", "mobile_cellular_subscriptions_total", "internet_users_total", 
                "broadband_fixed_subscriptions_total", "Total_Population", "Population_Growth_Rate", "Birth_Rate", 
                "Death_Rate", "Net_Migration_Rate", "Median_Age", "Sex_Ratio", "Infant_Mortality_Rate", 
                "Total_Fertility_Rate", "Total_Literacy_Rate", "Male_Literacy_Rate", "Female_Literacy_Rate", 
                "Youth_Unemployment_Rate", "Real_GDP_PPP_billion_USD", "GDP_Official_Exchange_Rate_billion_USD", 
                "Real_GDP_Growth_Rate_percent", "Real_GDP_per_Capita_USD", "Unemployment_Rate_percent", 
                "Youth_Unemployment_Rate_percent", "Budget_billion_USD", "Budget_Surplus_billion_USD", 
                "Budget_Deficit_percent_of_GDP", "Public_Debt_percent_of_GDP", "Exports_billion_USD", 
                "Imports_billion_USD", "Exchange_Rate_per_USD", "Population_Below_Poverty_Line_percent", 
                "electricity_access_percent", "electricity_generating_capacity_kW", "coal_metric_tons", 
                "petroleum_bbl_per_day", "refined_petroleum_products_bbl_per_day", "refined_petroleum_exports_bbl_per_day", 
                "refined_petroleum_imports_bbl_per_day", "natural_gas_cubic_meters", "carbon_dioxide_emissions_Mt", 
                "Area_Total", "Highest_Elevation", "Lowest_Elevation", "Forest_Land", "Other_Land", 
                "Agricultural_Land", "Arable_Land_percent"
            };
            
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
            
            // Custom Graph Builder
            renderCustomGraphBuilder();
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
                    std::string metricName = (selectedChoroplethColumn >= 0 && selectedChoroplethColumn < (int)availableColumns.size())
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
                        strncpy_s(searchBuffer, country.c_str(), sizeof(searchBuffer) - 1);
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
    ImGui::TextDisabled("Color map by:");
    
    // Build labels for availableColumns (assume order matches radarSpec.series.columns/labels)
    std::vector<std::string> choroplethLabels;
    for (const auto& col : availableColumns) {
        auto it = std::find(radarSpec.series.columns.begin(), radarSpec.series.columns.end(), col);
        if (it != radarSpec.series.columns.end()) {
            size_t idx = std::distance(radarSpec.series.columns.begin(), it);
            if (idx < radarSpec.series.labels.size())
                choroplethLabels.push_back(radarSpec.series.labels[idx]);
            else
                choroplethLabels.push_back(col);
        } else {
            choroplethLabels.push_back(col);
        }
    }

    const char* previewValue = (selectedChoroplethColumn >= 0 && selectedChoroplethColumn < static_cast<int>(choroplethLabels.size()))
        ? choroplethLabels[selectedChoroplethColumn].c_str()
        : "Select metric...";

    if (ImGui::BeginCombo("##ChoroplethMetric", previewValue)) {
        for (int i = 0; i < static_cast<int>(choroplethLabels.size()); ++i) {
            bool isSelected = (selectedChoroplethColumn == i);
            if (ImGui::Selectable(choroplethLabels[i].c_str(), isSelected)) {
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
            float t = (float)i / (float)(segments - 1);
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

    addSeparatorText("Search");
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

void UILayer::renderCustomGraphBuilder() {
    const auto& selectedCountries = mapLayer->getSelectedCountries();
    
    // Get country names from selected IDs
    std::vector<std::string> countryNames;
    countryNames.reserve(selectedCountries.size());
    for (const auto& id : selectedCountries) {
        countryNames.push_back(mapLayer->getCountryName(id));
    }
    
    // Only get hover country if mouse is not over UI elements
    std::string hoverCountry = "";
    if (!ImGui::GetIO().WantCaptureMouse) {
        hoverCountry = mapLayer->getCountryAtCursor();
        if (!hoverCountry.empty()) {
            hoverCountry = mapLayer->getCountryName(hoverCountry);
        }
    }
    
    // Render all saved custom graphs first
    for (size_t graphIdx = 0; graphIdx < savedCustomGraphs.size(); ++graphIdx) {
        // Use unique string ID for each custom graph to ensure drag-and-drop works independently
        std::string uniqueId = "CustomGraph_" + std::to_string(graphIdx);
        ImGui::PushID(uniqueId.c_str());
        
        auto customData = dataManager->collectSeries(countryNames, savedCustomGraphs[graphIdx].series.columns);
        
        // Add delete button
        if (ImGui::Button("Delete Graph")) {
            savedCustomGraphs.erase(savedCustomGraphs.begin() + graphIdx);
            ImGui::PopID();
            break;
        }
        ImGui::SameLine();
        ImGui::Text("%s", savedCustomGraphs[graphIdx].title.c_str());
        
        if (savedCustomGraphs[graphIdx].type == GraphType::TreeMap) {
            // Build continent map for treemap
            std::unordered_map<std::string, std::string> continentMap;
            for (const auto& country : countryNames) {
                continentMap[country] = dataManager->getContinent(country);
            }
            GraphRenderer::Render(savedCustomGraphs[graphIdx], countryNames, customData, hoverCountry, continentMap);
        } else {
            GraphRenderer::Render(savedCustomGraphs[graphIdx], countryNames, customData, hoverCountry);
        }
        
        ImGui::Separator();
        ImGui::PopID();
    }
    
    // Now show the builder menu
    addSeparatorText("Custom Graph Builder");
    
    if (selectedCountries.empty()) {
        ImGui::TextDisabled("Select countries to create custom graphs");
        return;
    }
    
    // Initialize selectedAttributes if needed
    if (selectedAttributes.size() != availableColumns.size()) {
        selectedAttributes.resize(availableColumns.size(), false);
    }
    
    // Graph Type Selection
    const char* graphTypes[] = {"Scatter Plot", "SPLOM", "Radar Chart", "TreeMap"};
    ImGui::Text("Graph Type:");
    ImGui::SetNextItemWidth(200);
    ImGui::Combo("##GraphType", &customGraphType, graphTypes, IM_ARRAYSIZE(graphTypes));
    
    ImGui::Spacing();
    
    // Attribute Selection
    ImGui::Text("Select Attributes:");
    
    if (ImGui::BeginChild("AttributeSelection", ImVec2(0, 200), true)) {
        for (size_t i = 0; i < availableColumns.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            bool isSelected = selectedAttributes[i];
            if (ImGui::Checkbox(availableColumns[i].c_str(), &isSelected)) {
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
    
    ImGui::Text("Selected: %d attributes", selectedCount);
    
    // Show requirements for selected graph type
    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Requirements:");
    ImGui::SameLine();
    switch (customGraphType) {
        case 0: // Scatter
            ImGui::Text("Exactly 2 attributes");
            break;
        case 1: // SPLOM
            ImGui::Text("At least 2 attributes");
            break;
        case 2: // Radar
            ImGui::Text("At least 3 attributes");
            break;
        case 3: // TreeMap
            ImGui::Text("Exactly 1 attribute");
            break;
    }
    
    // Check if requirements are met
    bool requirementsMet = false;
    std::string errorMsg = "";
    switch (customGraphType) {
        case 0: // Scatter
            requirementsMet = (selectedCount == 2);
            if (!requirementsMet && selectedCount > 0) {
                errorMsg = "Scatter Plot requires exactly 2 attributes";
            }
            break;
        case 1: // SPLOM
            requirementsMet = (selectedCount >= 2);
            if (!requirementsMet && selectedCount > 0) {
                errorMsg = "SPLOM requires at least 2 attributes";
            }
            break;
        case 2: // Radar
            requirementsMet = (selectedCount >= 3);
            if (!requirementsMet && selectedCount > 0) {
                errorMsg = "Radar Chart requires at least 3 attributes";
            }
            break;
        case 3: // TreeMap
            requirementsMet = (selectedCount == 1);
            if (!requirementsMet && selectedCount > 0) {
                errorMsg = "TreeMap requires exactly 1 attribute";
            }
            break;
    }
    
    if (!errorMsg.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", errorMsg.c_str());
    }
    
    // Generate button
    ImGui::Spacing();
    if (!requirementsMet) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button("Generate Graph", ImVec2(200, 0))) {
        if (selectedCount > 0) {
            // Build column list from selected attributes
            std::vector<std::string> selectedCols;
            std::vector<std::string> selectedLabels;
            
            for (size_t i = 0; i < availableColumns.size(); ++i) {
                if (selectedAttributes[i]) {
                    selectedCols.push_back(availableColumns[i]);
                    selectedLabels.push_back(availableColumns[i]);
                }
            }
            
            // Create new graph spec
            GraphSpec newGraph;
            newGraph.series.columns = selectedCols;
            newGraph.series.labels = selectedLabels;
            
            // Set graph type and title
            switch (customGraphType) {
                case 0: 
                    newGraph.type = GraphType::Scatter;
                    newGraph.title = "Custom Scatter Plot";
                    break;
                case 1: 
                    newGraph.type = GraphType::SPLOM;
                    newGraph.title = "Custom SPLOM";
                    break;
                case 2: 
                    newGraph.type = GraphType::Radar;
                    newGraph.title = "Custom Radar Chart";
                    break;
                case 3: 
                    newGraph.type = GraphType::TreeMap;
                    newGraph.title = "Custom TreeMap";
                    break;
            }
            
            newGraph.xLabel = "Countries";
            newGraph.yLabel = "Values";
            
            // Add to saved graphs
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
    
    // Clear all graphs button
    if (!savedCustomGraphs.empty()) {
        ImGui::SameLine();
        if (ImGui::Button("Clear All Graphs", ImVec2(200, 0))) {
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
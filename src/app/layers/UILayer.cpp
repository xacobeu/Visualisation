#include "UILayer.hpp"
#include "MapLayer.hpp"
#include "../util/GraphRenderer.hpp"

#include <implot.h>
#include <imgui.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <limits>
#include <unordered_set>

UILayer::UILayer(MapLayer* mapLayer) : mapLayer(mapLayer) {
    dataManager = std::make_unique<DataHandler>();

    // Radar Graph Spec
    radarSpec.type = GraphType::Radar;
    radarSpec.title = "Radar Chart";
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
    splomSpec.title = "Scatterplot Matrix (SPLOM)";
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
    treemapSpec.title = "Treemap";
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
    std::string searchCountry = "";
    
    // Check Search Bar 
    if (strlen(searchBuffer) > 0) {
         std::string s(searchBuffer);
         // Check if the buffer matches a known country exactly
         if (std::find(cachedCountryNames.begin(), cachedCountryNames.end(), s) != cachedCountryNames.end()) {
             searchCountry = s;
         } else {
             // Try partial match - only highlight if there's exactly one match
             std::string query = s;
             std::transform(query.begin(), query.end(), query.begin(), ::tolower);
             std::vector<std::string> matches;
             for (const auto& country : cachedCountryNames) {
                 std::string countryLower = country;
                 std::transform(countryLower.begin(), countryLower.end(), countryLower.begin(), ::tolower);
                 if (countryLower.find(query) != std::string::npos) {
                     matches.push_back(country);
                 }
             }
             // Only use partial match if there's exactly one result
             if (matches.size() == 1) {
                 searchCountry = matches[0];
             }
         }
    }

    // Check Mouse Hover 
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
    
    // Update map with brushed selection and filtered countries from graphs
    std::unordered_set<std::string> highlightedIsos;
    
    // Get the same sorted countryNames as used in graphs
    std::vector<std::string> countryNames;
    const auto& selectedIds = mapLayer->getSelectedCountries();
    countryNames.reserve(selectedIds.size());
    for (const auto& id : selectedIds) {
        countryNames.push_back(mapLayer->getCountryName(id));
    }
    std::sort(countryNames.begin(), countryNames.end());
    
    // Add brushed countries 
    if (GraphRenderer::s_HasSelection && !GraphRenderer::s_HighlightedPoints.empty()) {
        for (size_t i = 0; i < countryNames.size() && i < GraphRenderer::s_HighlightedPoints.size(); ++i) {
            if (GraphRenderer::s_HighlightedPoints[i]) {
                std::string iso = mapLayer->getIsoCodeFromName(countryNames[i]);
                if (!iso.empty()) {
                    highlightedIsos.insert(iso);
                }
            }
        }
    }
    
    // Add filtered countries 
    if (GraphRenderer::s_HasFilter && !GraphRenderer::s_FilteredPoints.empty()) {
        for (size_t i = 0; i < countryNames.size() && i < GraphRenderer::s_FilteredPoints.size(); ++i) {
            if (GraphRenderer::s_FilteredPoints[i]) {
                std::string iso = mapLayer->getIsoCodeFromName(countryNames[i]);
                if (!iso.empty()) {
                    highlightedIsos.insert(iso);
                }
            }
        }
    }
    
    mapLayer->setHighlightedCountries(highlightedIsos);

    // --- Main Panel: Graphs ---
    if (ImGui::Begin("Graphs")) {
        const auto& selectedIds = mapLayer->getSelectedCountries();
        std::vector<std::string> countryNames;
        if (!selectedIds.empty()) {
            countryNames.reserve(selectedIds.size());
            for (const auto& id : selectedIds) {
                countryNames.push_back(mapLayer->getCountryName(id));
            }
            std::sort(countryNames.begin(), countryNames.end());
        }

        const float sidebarWidth = 280.0f;
        ImGui::BeginChild("GraphsSidebar", ImVec2(sidebarWidth, 0), true);
        renderSearchBar();
        ImGui::Separator();
        renderGraphFilterSection(countryNames);
        ImGui::Separator();
        renderCategoricalFilterSection(countryNames);
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("GraphsContent", ImVec2(0, 0), false);

        if (selectedIds.empty()) {
            ImGui::TextDisabled("Click on the map to select countries.");
        } else {
        
            std::vector<std::string> splomAllColumns = splomSpec.series.columns;
            for (const auto& col : availableColumns) {
                if (std::find(splomSpec.series.columns.begin(), splomSpec.series.columns.end(), col) == splomSpec.series.columns.end()) {
                    splomAllColumns.push_back(col);
                }
            }
            auto splomData = dataManager->collectSeries(countryNames, splomAllColumns);
            GraphRenderer::Render(splomSpec, countryNames, splomData, hoverCountry);

            std::vector<std::string> radarAllColumns = radarSpec.series.columns;
            for (const auto& col : availableColumns) {
                if (std::find(radarSpec.series.columns.begin(), radarSpec.series.columns.end(), col) == radarSpec.series.columns.end()) {
                    radarAllColumns.push_back(col);
                }
            }
            auto radarData = dataManager->collectSeries(countryNames, radarAllColumns);
            GraphRenderer::Render(radarSpec, countryNames, radarData, hoverCountry);

            // Treemap
            addSeparatorText("Treemap");
            const char* treemapMetrics[] = {
                "Fixed Telephone Subscriptions", "Mobile Cellular Subscriptions", "Total Internet Users", 
                "Fixed Broadband Subscriptions", "Total Population", "Sex Ratio", "Infant Mortality Rate", 
                "Total Fertility Rate", "Total Literacy Rate", "Male Literacy Rate", "Female Literacy Rate", 
                "Real GDP (PPP) $B", "GDP (Official Exchange) $B", 
                "Budget $B", "Budget Surplus $B", "Exports $B", 
                "Imports $B", 
                "Electricity Generating Capacity kW", "Coal (Metric Tons)", 
                "Petroleum (bbl/day)", "Refined Petroleum Products (bbl/day)", "Refined Petroleum Exports (bbl/day)", 
                "Refined Petroleum Imports (bbl/day)", "Natural Gas (cu m)", "CO2 Emissions (Mt)", 
                "Total Area", "Forest Land", "Other Land", 
                "Agricultural Land",
            };
            const char* treemapColumns[] = {
                "telephone_fixed_subscriptions_total", "mobile_cellular_subscriptions_total", "internet_users_total", 
                "broadband_fixed_subscriptions_total", "Total_Population", "Sex_Ratio", "Infant_Mortality_Rate", 
                "Total_Fertility_Rate", "Total_Literacy_Rate", "Male_Literacy_Rate", "Female_Literacy_Rate", 
                "Real_GDP_PPP_billion_USD", "GDP_Official_Exchange_Rate_billion_USD", 
                "Budget_billion_USD", "Budget_Surplus_billion_USD", "Exports_billion_USD", 
                "Imports_billion_USD", 
                "electricity_generating_capacity_kW", "coal_metric_tons", 
                "petroleum_bbl_per_day", "refined_petroleum_products_bbl_per_day", "refined_petroleum_exports_bbl_per_day", 
                "refined_petroleum_imports_bbl_per_day", "natural_gas_cubic_meters", "carbon_dioxide_emissions_Mt", 
                "Area_Total", "Forest_Land", "Other_Land", 
                "Agricultural_Land",
            };
            
            ImGui::Text("Select Metric for TreeMap:");
            
            if (ImGui::Combo("", &selectedTreemapMetric, treemapMetrics, IM_ARRAYSIZE(treemapMetrics))) {
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

        ImGui::EndChild();
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
    ImGui::SetNextItemWidth(-1); 
    if (ImGui::InputTextWithHint("##Search", "Filter active graphs...", searchBuffer, IM_ARRAYSIZE(searchBuffer))) {
      
    }
    
    // Dropdown suggestions - Only search WITHIN selected countries
    if (strlen(searchBuffer) > 0) {
        // Only show popup if we have selected countries
        const auto& selectedIds = mapLayer->getSelectedCountries();
        if (selectedIds.empty()) return;

        if (ImGui::BeginChild("SearchResults", ImVec2(0, 100), true)) {
            std::string query = searchBuffer;
            std::transform(query.begin(), query.end(), query.begin(), ::tolower);
            
            // Build temporary list of names from selected IDs
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

void UILayer::renderGraphFilterSection(const std::vector<std::string>& countryNames) {
    addSeparatorText("Numeric Filters");

    if (availableColumns.empty()) {
        ImGui::TextDisabled("No metrics available for filtering.");
        GraphRenderer::s_HasFilter = false;
        GraphRenderer::s_FilteredPoints.clear();
        return;
    }

    ImGui::Checkbox("Enable numeric filters", &graphFilterEnabled);

    // Prepare column labels
    std::vector<std::string> columnLabels;
    columnLabels.reserve(availableColumns.size());
    for (const auto& col : availableColumns) {
        auto it = std::find(radarSpec.series.columns.begin(), radarSpec.series.columns.end(), col);
        if (it != radarSpec.series.columns.end()) {
            size_t idx = std::distance(radarSpec.series.columns.begin(), it);
            if (idx < radarSpec.series.labels.size()) {
                columnLabels.push_back(radarSpec.series.labels[idx]);
            } else {
                columnLabels.push_back(col);
            }
        } else {
            columnLabels.push_back(col);
        }
    }

    if (!graphFilterEnabled || countryNames.empty()) {
        if (countryNames.empty()) {
            ImGui::TextDisabled("Select countries to apply filters.");
        }
        // Clear numeric filter highlighting when disabled
        if (!categoricalFilterEnabled) {
            GraphRenderer::s_HasFilter = false;
            GraphRenderer::s_FilteredPoints.clear();
        }
        return;
    }
    
    // Add new filter button
    if (ImGui::Button("+ Add Filter##Numeric")) {
        numericFilters.push_back(NumericFilter());
    }
    
    if (numericFilters.empty()) {
        ImGui::TextDisabled("Click '+ Add Filter' to create a filter.");
        // Clear filter state if no filters remain and categorical is also empty/disabled
        if (!categoricalFilterEnabled || categoricalFilters.empty()) {
            GraphRenderer::s_HasFilter = false;
            GraphRenderer::s_FilteredPoints.clear();
        }
        return;
    }
    
    ImGui::Separator();
    
    // Display each filter
    int filterToRemove = -1;
    int totalMatchCount = 0;
    
    for (size_t filterIdx = 0; filterIdx < numericFilters.size(); ++filterIdx) {
        auto& filter = numericFilters[filterIdx];
        
        ImGui::PushID(static_cast<int>(filterIdx));
        ImGui::Text("Filter %zu", filterIdx + 1);
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - 60);
        if (ImGui::SmallButton("Remove")) {
            filterToRemove = static_cast<int>(filterIdx);
        }
        
        // Ensure column index is valid
        if (filter.columnIndex < 0 || filter.columnIndex >= static_cast<int>(availableColumns.size())) {
            filter.columnIndex = 0;
        }
        
        const char* previewValue = (filter.columnIndex >= 0 && filter.columnIndex < static_cast<int>(columnLabels.size()))
            ? columnLabels[filter.columnIndex].c_str()
            : "Select metric...";
        
        // Metric selector
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##Metric", previewValue)) {
            for (int i = 0; i < static_cast<int>(columnLabels.size()); ++i) {
                bool isSelected = (filter.columnIndex == i);
                if (ImGui::Selectable(columnLabels[i].c_str(), isSelected)) {
                    filter.columnIndex = i;
                    filter.lastColumnIndex = -1; // Trigger data reload
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        
        const std::string& columnName = availableColumns[filter.columnIndex];
        auto columnData = dataManager->getColumnWithUnitForAllCountries(columnName);
        
        if (columnData.values.empty()) {
            ImGui::TextDisabled("No numeric data available for this metric.");
            ImGui::PopID();
            continue;
        }
        
        // Calculate min/max from data
        float dataMin = std::numeric_limits<float>::max();
        float dataMax = std::numeric_limits<float>::lowest();
        for (const auto& entry : columnData.values) {
            dataMin = std::min(dataMin, entry.second);
            dataMax = std::max(dataMax, entry.second);
        }
        
        // Initialize range if column changed
        if (filter.lastColumnIndex != filter.columnIndex) {
            filter.minValue = dataMin;
            filter.maxValue = dataMax;
            filter.dataMin = dataMin;
            filter.dataMax = dataMax;
            filter.unit = columnData.unit;
            filter.lastColumnIndex = filter.columnIndex;
        }
        
        const float range = std::max(0.01f, dataMax - dataMin);
        const float step = range / 200.0f;
        
        ImGui::SetNextItemWidth(-1);
        ImGui::DragFloatRange2("##ValueRange", &filter.minValue, &filter.maxValue, step, dataMin, dataMax, "Min: %.2f", "Max: %.2f");
        if (filter.minValue > filter.maxValue) {
            std::swap(filter.minValue, filter.maxValue);
        }
        
        ImGui::Text("Unit: %s", filter.unit.empty() ? "n/a" : filter.unit.c_str());
        
        if (filterIdx < numericFilters.size() - 1) {
            ImGui::Separator();
        }
        
        ImGui::PopID();
    }
    
    // Remove filter if requested
    if (filterToRemove >= 0) {
        numericFilters.erase(numericFilters.begin() + filterToRemove);
        // If all numeric filters removed and no categorical filters, clear highlighting
        if (numericFilters.empty() && !categoricalFilterEnabled) {
            GraphRenderer::s_HasFilter = false;
            GraphRenderer::s_FilteredPoints.clear();
        }
    }
    
    // Apply all numeric filters 
    if (GraphRenderer::s_FilteredPoints.size() != countryNames.size()) {
        GraphRenderer::s_FilteredPoints.assign(countryNames.size(), false);
    }
    
    for (size_t i = 0; i < countryNames.size(); ++i) {
        bool matchesAll = true;
        
        // Check if country matches ALL numeric filters
        for (const auto& filter : numericFilters) {
            const std::string& columnName = availableColumns[filter.columnIndex];
            auto columnData = dataManager->getColumnWithUnitForAllCountries(columnName);
            
            auto it = columnData.values.find(countryNames[i]);
            if (it == columnData.values.end() || it->second < filter.minValue || it->second > filter.maxValue) {
                matchesAll = false;
                break;
            }
        }
        
        GraphRenderer::s_FilteredPoints[i] = matchesAll;
        if (matchesAll) {
            totalMatchCount++;
        }
    }
    
    GraphRenderer::s_HasFilter = totalMatchCount > 0;
    
    ImGui::Separator();
    ImGui::Text("Total Matches: %d / %zu", totalMatchCount, countryNames.size());
}

void UILayer::renderCategoricalFilterSection(const std::vector<std::string>& countryNames) {
    addSeparatorText("Categorical Filters");
    
    ImGui::Checkbox("Enable categorical filters", &categoricalFilterEnabled);
    
    // Define available categorical columns
    static const std::vector<std::pair<std::string, std::string>> categoricalColumns = {
        {"Continent", "Continent"},
        {"Government_Type", "Government Type"}
    };
    
    if (!categoricalFilterEnabled || countryNames.empty()) {
        if (countryNames.empty()) {
            ImGui::TextDisabled("Select countries to apply filters.");
        }
        // Clear categorical filter highlighting when disabled
        if (!graphFilterEnabled) {
            GraphRenderer::s_HasFilter = false;
            GraphRenderer::s_FilteredPoints.clear();
        }
        return;
    }
    
    // Add new filter button
    if (ImGui::Button("+ Add Filter##Categorical")) {
        categoricalFilters.push_back(CategoricalFilter());
    }
    
    if (categoricalFilters.empty()) {
        ImGui::TextDisabled("Click '+ Add Filter' to create a filter.");
        // Clear filter state if no filters remain and numeric is also empty/disabled
        if (!graphFilterEnabled || numericFilters.empty()) {
            GraphRenderer::s_HasFilter = false;
            GraphRenderer::s_FilteredPoints.clear();
        }
        return;
    }
    
    ImGui::Separator();
    
    // Display each filter
    int filterToRemove = -1;
    int totalMatchCount = 0;
    
    for (size_t filterIdx = 0; filterIdx < categoricalFilters.size(); ++filterIdx) {
        auto& filter = categoricalFilters[filterIdx];
        
        ImGui::PushID(static_cast<int>(filterIdx));
        
        // Filter header with remove button
        ImGui::Text("Filter %zu", filterIdx + 1);
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - 60);
        if (ImGui::SmallButton("Remove")) {
            filterToRemove = static_cast<int>(filterIdx);
        }
        
        // Column selector
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##Category", categoricalColumns[filter.columnIndex].second.c_str())) {
            for (int i = 0; i < static_cast<int>(categoricalColumns.size()); ++i) {
                bool isSelected = (filter.columnIndex == i);
                if (ImGui::Selectable(categoricalColumns[i].second.c_str(), isSelected)) {
                    filter.columnIndex = i;
                    filter.values.clear();
                    filter.selected.clear();
                }
                if (isSelected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        
        const std::string& columnName = categoricalColumns[filter.columnIndex].first;
        
        // Collect unique values for this categorical column
        if (filter.values.empty()) {
            std::unordered_set<std::string> uniqueValues;
            for (const auto& country : countryNames) {
                if (columnName == "Continent") {
                    std::string continent = dataManager->getContinent(country);
                    if (!continent.empty() && continent != "Unknown") {
                        uniqueValues.insert(continent);
                    }
                } else if (columnName == "Government_Type") {
                    std::string govType = dataManager->getCategoricalValue(country, "Government_Type");
                    if (!govType.empty()) {
                        uniqueValues.insert(govType);
                    }
                }
            }
            filter.values = std::vector<std::string>(uniqueValues.begin(), uniqueValues.end());
            std::sort(filter.values.begin(), filter.values.end());
            filter.selected.assign(filter.values.size(), true); // Start with all selected
        }
        
        // Display checkboxes for each value
        if (ImGui::BeginChild("##Values", ImVec2(0, 100), true)) {
            for (size_t i = 0; i < filter.values.size(); ++i) {
                ImGui::PushID(static_cast<int>(i));
                bool selected = filter.selected[i];
                if (ImGui::Checkbox(filter.values[i].c_str(), &selected)) {
                    filter.selected[i] = selected;
                }
                ImGui::PopID();
            }
        }
        ImGui::EndChild();
        
        if (filterIdx < categoricalFilters.size() - 1) {
            ImGui::Separator();
        }
        
        ImGui::PopID();
    }
    
    // Remove filter if requested
    if (filterToRemove >= 0) {
        categoricalFilters.erase(categoricalFilters.begin() + filterToRemove);
        // If all categorical filters removed and no numeric filters, clear highlighting
        if (categoricalFilters.empty() && !graphFilterEnabled) {
            GraphRenderer::s_HasFilter = false;
            GraphRenderer::s_FilteredPoints.clear();
        }
    }
    
    // Apply all categorical filters (AND logic)
    if (GraphRenderer::s_FilteredPoints.size() != countryNames.size()) {
        GraphRenderer::s_FilteredPoints.assign(countryNames.size(), false);
    }
    
    for (size_t i = 0; i < countryNames.size(); ++i) {
        bool matchesAll = true;
        
        // Check if country matches all filters
        for (const auto& filter : categoricalFilters) {
            const std::string& columnName = categoricalColumns[filter.columnIndex].first;
            bool matchesThisFilter = false;
            
            if (columnName == "Continent") {
                std::string continent = dataManager->getContinent(countryNames[i]);
                for (size_t j = 0; j < filter.values.size(); ++j) {
                    if (filter.selected[j] && continent == filter.values[j]) {
                        matchesThisFilter = true;
                        break;
                    }
                }
            } else if (columnName == "Government_Type") {
                std::string govType = dataManager->getCategoricalValue(countryNames[i], "Government_Type");
                for (size_t j = 0; j < filter.values.size(); ++j) {
                    if (filter.selected[j] && govType == filter.values[j]) {
                        matchesThisFilter = true;
                        break;
                    }
                }
            }
            
            if (!matchesThisFilter) {
                matchesAll = false;
                break;
            }
        }
        
        // Combine with numeric filter if both are enabled
        if (graphFilterEnabled && GraphRenderer::s_HasFilter) {
            GraphRenderer::s_FilteredPoints[i] = GraphRenderer::s_FilteredPoints[i] && matchesAll;
        } else {
            GraphRenderer::s_FilteredPoints[i] = matchesAll;
        }
        
        if (GraphRenderer::s_FilteredPoints[i]) {
            totalMatchCount++;
        }
    }
    
    GraphRenderer::s_HasFilter = totalMatchCount > 0;
    
    ImGui::Separator();
    ImGui::Text("Total Matches: %d / %zu", totalMatchCount, countryNames.size());
}

void UILayer::renderChoroplethControls() {
    ImGui::TextDisabled("Color map by:");
    
    // Build labels for availableColumns 
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
        for (int i = 0; i < (int)availableColumns.size(); ++i) {
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

        
        // Format strings first to calculate exact sizes
        char minBuf[32]; 
        snprintf(minBuf, sizeof(minBuf), "%.1f", currentMinVal);
        
        char maxBuf[32]; 
        snprintf(maxBuf, sizeof(maxBuf), "%.1f", currentMaxVal);

        float startX = ImGui::GetCursorPosX();

        // --- Left Label  ---
        ImGui::Text("%s", minBuf);

        // --- Center Label  ---
        if (currentIsDiverging) {
            const char* midStr = "0";
            float midTextWidth = ImGui::CalcTextSize(midStr).x;
            ImGui::SameLine();
            ImGui::SetCursorPosX(startX + (width * 0.5f) - (midTextWidth * 0.5f));
            ImGui::Text("%s", midStr);
        }

        // --- Right Label ---
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

void UILayer::renderCustomTreeMapBuilder(const std::string& hoverCountry) {
    const auto& selectedCountries = mapLayer->getSelectedCountries();
    
    // Get country names from selected IDs
    std::vector<std::string> countryNames;
    countryNames.reserve(selectedCountries.size());
    for (const auto& id : selectedCountries) {
        countryNames.push_back(mapLayer->getCountryName(id));
    }
    
    // Filter columns suitable for treemaps 
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
            // Exclude elevations
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
    
    // Initialize selectedAttributes if needed 
    if (selectedAttributes.size() != treemapCompatibleColumns.size()) {
        selectedAttributes.resize(treemapCompatibleColumns.size(), false);
    }
    
    // Track which single metric is selected (-1 = none)
    static int selectedTreemapMetric = -1;
    
    // Helper function to format column names into readable display names
    auto formatColumnName = [](const std::string& columnName) -> std::string {
        std::string formatted = columnName;
        // Replace underscores with spaces
        std::replace(formatted.begin(), formatted.end(), '_', ' ');
        return formatted;
    };
    
    ImGui::Spacing();
    
    // Metric Selection for TreeMap using dropdown
    ImGui::Text("Select Metric for TreeMap:");
    
    std::string previewValue = "Select metric...";
    if (selectedTreemapMetric >= 0 && selectedTreemapMetric < static_cast<int>(treemapCompatibleColumns.size())) {
        previewValue = formatColumnName(treemapCompatibleColumns[selectedTreemapMetric]);
    }
    
    ImGui::SetNextItemWidth(-1);
    if (ImGui::BeginCombo("##TreemapMetric", previewValue.c_str())) {
        for (int i = 0; i < static_cast<int>(treemapCompatibleColumns.size()); ++i) {
            bool isSelected = (selectedTreemapMetric == i);
            std::string displayName = formatColumnName(treemapCompatibleColumns[i]);
            if (ImGui::Selectable(displayName.c_str(), isSelected)) {
                selectedTreemapMetric = i;
                // Update selectedAttributes array to match
                std::fill(selectedAttributes.begin(), selectedAttributes.end(), false);
                if (selectedTreemapMetric >= 0) {
                    selectedAttributes[selectedTreemapMetric] = true;
                }
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    
    // Generate button
    ImGui::Spacing();

    if (ImGui::Button("Generate TreeMap", ImVec2(200, 0))) {
        
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

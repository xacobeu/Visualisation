#include "UILayer.hpp"
#include "MapLayer.hpp"

#include <imgui.h>
#include <algorithm>

UILayer::UILayer(MapLayer* mapLayer) : mapLayer(mapLayer) {
    dataManager = std::make_unique<DataHandler>();

    // Radar Graph Spec
    radarSpec.type = GraphType::Radar;
    radarSpec.title = "Communications Overview";
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
    splomSpec.title = "Economic Indicators SPLOM";
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

    // --- Main Panel: Graphs ---
    if (ImGui::Begin("Graphs")) {

        // Get Selected Countries from MapLayer.
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

            // SPLOM
            auto splomData = dataManager->collectSeries(countryNames, splomSpec.series.columns);
            GraphRenderer::Render(splomSpec, countryNames, splomData);

            ImGui::Separator();

            // Radar
            auto radarData = dataManager->collectSeries(countryNames, radarSpec.series.columns);
            GraphRenderer::Render(radarSpec, countryNames, radarData);

            ImGui::Separator();

            // TODO: Treemap

        }
    }
    ImGui::End();

    // --- Hover Tooltip for Map ---
    // Only show if mouse is not over ImGui windows
    if (!ImGui::GetIO().WantCaptureMouse) {
        std::string hoveredCountry = mapLayer->getCountryAtCursor();
        if (!hoveredCountry.empty()) {
            std::string countryName = mapLayer->getCountryName(hoveredCountry);
            
            ImGui::BeginTooltip();
            ImGui::Text("%s", countryName.c_str());
            
            // Show choropleth value if active
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

void UILayer::renderChoroplethControls() {
    ImGui::TextDisabled("Color map by data metric:");
    
    // Combo box for selecting the column
    const char* previewValue = (selectedChoroplethColumn >= 0 && selectedChoroplethColumn < (int)availableColumns.size())
        ? availableColumns[selectedChoroplethColumn].c_str()
        : "Select metric...";
    
    if (ImGui::BeginCombo("##ChoroplethMetric", previewValue)) {
        for (int i = 0; i < (int)availableColumns.size(); ++i) {
            bool isSelected = (selectedChoroplethColumn == i);
            if (ImGui::Selectable(availableColumns[i].c_str(), isSelected)) {
                selectedChoroplethColumn = i;
                
                // Apply choropleth with unit
                auto columnData = dataManager->getColumnWithUnitForAllCountries(availableColumns[i]);
                
                // Convert country names to ISO codes
                std::unordered_map<std::string, float> isoData;
                for (const auto& [countryName, value] : columnData.values) {
                    std::string isoCode = mapLayer->getIsoCodeFromName(countryName);
                    if (!isoCode.empty()) {
                        isoData[isoCode] = value;
                    }
                }
                
                mapLayer->applyChoropleth(isoData, columnData.unit);
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    
    // Clear button
    if (mapLayer->isChoroplethActive()) {
        
        // Color legend
        ImGui::Spacing();
        ImGui::TextDisabled("Legend:");
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float width = ImGui::GetContentRegionAvail().x;
        float height = 15.0f;
        
        // Draw gradient bar
        int segments = 50;
        float segWidth = width / segments;
        for (int i = 0; i < segments; ++i) {
            float t = (float)i / (segments - 1);
            
            // Two-color gradient: light blue -> dark blue
            float r = 0.8f - t * 0.7f;
            float g = 0.9f - t * 0.7f;
            float b = 1.0f - t * 0.4f;
            
            ImU32 col = IM_COL32((int)(r*255), (int)(g*255), (int)(b*255), 255);
            drawList->AddRectFilled(
                ImVec2(pos.x + i * segWidth, pos.y),
                ImVec2(pos.x + (i + 1) * segWidth, pos.y + height),
                col
            );
        }
        
        ImGui::Dummy(ImVec2(width, height));
        ImGui::TextDisabled("Low");
        ImGui::SameLine(width - ImGui::CalcTextSize("High").x);
        ImGui::TextDisabled("High");
    }
}

void UILayer::renderSelectionList() {
    addSeparatorText("Selected Countries");

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

                // Right-align the X button
                float buttonWidth = ImGui::CalcTextSize("X").x + ImGui::GetStyle().FramePadding.x * 2.0f;
                ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - buttonWidth);

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

                if (ImGui::SmallButton("X")) {
                    toDeselect = countryId;
                }

                ImGui::PopStyleColor(3);
                ImGui::PopID();
            }

            // Handle deselction after loop to avoid iterator invalidation
            if (!toDeselect.empty()) {
                mapLayer->deselectCountry(toDeselect);
            }
        }
        ImGui::EndChild();

        // Clear All button (only when there are selections)
        if (ImGui::Button("Clear All")) {
            mapLayer->clearSelection();
        }
        ImGui::SameLine();
    }

    // Select All button (always visible)
    if (!selectedCountries.empty()) {
        float buttonWidth = ImGui::CalcTextSize("Select All").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - buttonWidth - 7.0f);
    }
    if (ImGui::Button("Select All")) {
        mapLayer->selectAll();
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

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoSavedSettings |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("LoadingScreen", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    // Center the loading text
    ImVec2 windowSize = ImGui::GetWindowSize();

    const char* loadingText = "Loading Data...";
    ImVec2 textSize = ImGui::CalcTextSize(loadingText);

    ImGui::SetCursorPos(ImVec2((windowSize.x - textSize.x) * 0.5f, (windowSize.y - textSize.y) * 0.5f - 20.0f));
    ImGui::Text("%s", loadingText);

    // Add a spinner/progress indicator
    ImGui::SetCursorPos(ImVec2((windowSize.x) * 0.5f - 15.0f, (windowSize.y) * 0.5f + 10.0f));

    // Simple rotating spinner
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

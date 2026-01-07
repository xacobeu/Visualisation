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
        "telephone_fixed_subscriptions_total",
        "mobile_cellular_subscriptions_total",
        "internet_users_total",
        "broadband_fixed_subscriptions_total"
    };
    radarSpec.series.labels = {
        "Fixed Telephone",
        "Mobile Cellular",
        "Internet Users",
        "Broadband Fixed"
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

    // Bar Chart Spec
    barSpec.type = GraphType::Bar;
    barSpec.title = "Energy Consumption";
    barSpec.xLabel = "Countries";
    barSpec.yLabel = "Energy Metrics";
    barSpec.series.columns = {
        "electricity_generating_capacity_kW",
        "coal_metric_tons",
        "petroleum_bbl_per_day",
        "natural_gas_cubic_meters"
    };
    barSpec.series.labels = {
        "Electricity Capacity",
        "Coal",
        "Petroleum",
        "Natural Gas"
    };

    // Scatter Plot Spec
    scatterSpec.type = GraphType::Scatter;
    scatterSpec.title = "Geography vs Government";
    scatterSpec.xLabel = "Area (sq km)";
    scatterSpec.yLabel = "Suffrage Age (years)";
    scatterSpec.series.columns = {
        "Area_Total",
        "Suffrage_Age"
    };
    scatterSpec.series.labels = {
        "Area",
        "Suffrage Age"
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
        dataInitialized = true;
    }

    setupDockspace();

    // --- Side Panel: Country Selection ---
    if (ImGui::Begin("Country Selection")) {
        renderFPSDisplay();
        ImGui::Separator();
        renderSelectionList();
        ImGui::End();
    }

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

            // Radar
            auto radarData = dataManager->collectSeries(countryNames, radarSpec.series.columns);
            GraphRenderer::Render(radarSpec, countryNames, radarData);

            ImGui::Separator();

            // SPLOM
            auto splomData = dataManager->collectSeries(countryNames, splomSpec.series.columns);
            GraphRenderer::Render(splomSpec, countryNames, splomData);

            ImGui::Separator();

            // Bar Chart
            auto barData = dataManager->collectSeries(countryNames, barSpec.series.columns);
            GraphRenderer::Render(barSpec, countryNames, barData);

            ImGui::Separator();

            // Scatter Plot
            auto scatterData = dataManager->collectSeries(countryNames, scatterSpec.series.columns);
            GraphRenderer::Render(scatterSpec, countryNames, scatterData);
        }
        ImGui::End();
    }

}

void UILayer::renderSelectionList() {
    addSeparatorText("Selected Countries");

    const auto& selectedCountries = mapLayer->getSelectedCountries();

    if (selectedCountries.empty()) {
        ImGui::TextDisabled("No countries selected (Click on countries to select them)");
    } else {
        ImGui::BeginChild("CountryList", ImVec2(0, 200), true);

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
        ImGui::EndChild();

        // Handle deselction after loop to avoid iterator invalidation
        if (!toDeselect.empty()) {
            mapLayer->deselectCountry(toDeselect);
        }

        // Action Buttons
        if (ImGui::Button("Clear All")) {
            mapLayer->clearSelection();
        }

        float buttonWidth = ImGui::CalcTextSize("Clear All").x + ImGui::GetStyle().FramePadding.x * 2.0f;
        ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - buttonWidth - 7.0f);

        if (ImGui::Button("Select All")) {
            mapLayer->selectAll();
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

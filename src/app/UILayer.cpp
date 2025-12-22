#include "UILayer.hpp"
#include "MapLayer.hpp"
#include <imgui.h>
#include <implot.h>

void UILayer::onUpdate(float deltaTime) {
    setupDockspace();

    ImGui::Begin("Controls");

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    ImGui::Separator();

    // Selected countries list (only if MapLayer is available)
    if (mapLayer) {
        ImGui::SeparatorText("Selected Countries");

        const auto& selectedCountries = mapLayer->getSelectedCountries();

        if (selectedCountries.empty()) {
            ImGui::TextDisabled("No countries selected");
            ImGui::Text("Click on countries to select them");
        } else {
            ImGui::Text("Total selected: %zu", selectedCountries.size());

            ImGui::BeginChild("CountryList", ImVec2(0, 200), true);

            // Display each selected country with a remove button
            std::string toDeselect = "";
            for (const auto& countryId : selectedCountries) {
                std::string countryName = mapLayer->getCountryName(countryId);

                ImGui::PushID(countryId.c_str());

                // Show country name
                ImGui::Text("%s (%s)", countryName.c_str(), countryId.c_str());

                ImGui::SameLine();

                // Add a small remove button
                if (ImGui::SmallButton("X")) {
                    toDeselect = countryId;
                }

                ImGui::PopID();
            }

            ImGui::EndChild();

            // Deselect after iteration to avoid modifying the set while iterating
            if (!toDeselect.empty()) {
                mapLayer->deselectCountry(toDeselect);
            }

            // Add a "Clear All" button
            if (ImGui::Button("Clear All")) {
                mapLayer->clearSelection();
            }
        }
    }

    ImGui::End();

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

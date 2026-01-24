#include "GraphRenderer.hpp"

#include <imgui.h>
#include <implot.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <iostream>

// Static member definitions
std::vector<GraphRenderer::RadarFeature> GraphRenderer::g_AvailableFeatures;
std::vector<GraphRenderer::RadarFeature> GraphRenderer::g_ActiveFeatures;

// SPLOM drag selection static members
bool GraphRenderer::s_IsDragging = false;
bool GraphRenderer::s_HasSelection = false;
float GraphRenderer::s_DragStartX = 0.0f;
float GraphRenderer::s_DragStartY = 0.0f;
float GraphRenderer::s_DragEndX = 0.0f;
float GraphRenderer::s_DragEndY = 0.0f;
int GraphRenderer::s_DragPlotRow = -1;
int GraphRenderer::s_DragPlotCol = -1;
std::vector<bool> GraphRenderer::s_HighlightedPoints;

void GraphRenderer::Render(const GraphSpec& spec,
                           const std::vector<std::string>& itemLabels,
                           std::vector<PlotSeries>& data)
{
    if (itemLabels.empty() || data.empty()) {
        ImGui::TextDisabled("No data selected to render.");
        return;
    }

    switch (spec.type) {
        case GraphType::SPLOM:
            RenderSPLOM(spec, itemLabels, data);
            return;
        case GraphType::Radar:
            RenderRadar(spec, itemLabels, data);
            return;
        case GraphType::Bar:
        case GraphType::Scatter:
            break;
    }

    // Generic Setup for Bar and Scatter plots
    ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(15, 80));
    ImPlot::PushStyleVar(ImPlotStyleVar_FitPadding, (spec.type == GraphType::Bar) ? ImVec2(0.2f, 0.0f) : ImVec2(0.2f, 0.2f));
    ImPlot::PushStyleColor(ImPlotCol_FrameBg, ImVec4(0,0,0,0));

    if (ImPlot::BeginPlot(spec.title.c_str(), ImVec2(-1, 400))) {

        ImPlot::SetupAxes(
            spec.xLabel.c_str(), spec.yLabel.c_str(),
            ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoHighlight | (spec.type == GraphType::Bar ? ImPlotAxisFlags_NoTickLabels : 0),
            ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoHighlight
        );

        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, ImPlotCond_Once);
        ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);

        if (spec.type == GraphType::Bar) {
            RenderBar(spec, itemLabels, data);
        } else if (spec.type == GraphType::Scatter) {
            RenderScatter(spec, itemLabels, data);
        }

        ImPlot::EndPlot();
    }

    ImPlot::PopStyleColor();
    ImPlot::PopStyleVar(2);
}

void GraphRenderer::RenderBar(const GraphSpec& spec,
                              const std::vector<std::string>& labels,
                              const std::vector<PlotSeries>& data)
{
    std::vector<float> x(labels.size());
    for (size_t i = 0; i < x.size(); ++i)
        x[i] = (float)i;

    for (size_t i = 0; i < data.size(); ++i) {
        const auto& s = data[i];

        float barWidth = 0.8f / data.size();
        float offset = (i - (data.size() - 1) / 2.0f) * barWidth;

        std::vector<float> xOffset(x.size());
        for (size_t j = 0; j < x.size(); ++j)
            xOffset[j] = x[j] + offset;

        ImPlot::PlotBars(
            spec.series.labels[i].c_str(),
            xOffset.data(),
            s.values.data(),
            (int)xOffset.size(),
            barWidth
        );
    }

    DrawRotatedLabels(labels);
}

void GraphRenderer::RenderScatter(const GraphSpec&,
                                  const std::vector<std::string>& labels,
                                  const std::vector<PlotSeries>& data)
{
    if (data.size() < 2) {
        ImGui::Text("Scatter plot requires at least 2 data series (X and Y).");
        return;
    }

    ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Cross);

    for (size_t i = 0; i < labels.size(); ++i) {
        ImPlot::PlotScatter(
            labels[i].c_str(),
            &data[0].values[i],
            &data[1].values[i],
            1
        );
    }

    ImPlot::PopStyleVar();

    // Tooltip logic
    if (ImPlot::IsPlotHovered()) {
        ImPlotPoint mouse = ImPlot::GetPlotMousePos();
        float minDist = FLT_MAX;
        int closestIdx = -1;

        for (size_t i = 0; i < labels.size(); ++i) {
            ImVec2 dataPix = ImPlot::PlotToPixels(data[0].values[i], data[1].values[i]);
            ImVec2 mousePix = ImPlot::PlotToPixels(mouse);

            float dx = mousePix.x - dataPix.x;
            float dy = mousePix.y - dataPix.y;
            float dist = dx * dx + dy * dy;

            if (dist < minDist) {
                minDist = dist;
                closestIdx = (int)i;
            }
        }

        if (closestIdx >= 0 && minDist < 400.0f) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", labels[closestIdx].c_str());
            ImGui::Text("X: %.2f", data[0].values[closestIdx]);
            ImGui::Text("Y: %.2f", data[1].values[closestIdx]);
            ImGui::EndTooltip();
        }
    }
}

void GraphRenderer::RenderSPLOM(const GraphSpec& spec,
                                const std::vector<std::string>& labels,
                                const std::vector<PlotSeries>& data)
{
    const int n = static_cast<int>(data.size());
    if (n < 2) {
        ImGui::Text("SPLOM requires at least 2 data series.");
        return;
    }

    ImGui::TextUnformatted(spec.title.c_str());

    std::vector<float> seriesMin(n), seriesMax(n);
    for (int i = 0; i < n; ++i) {
        if (data[i].values.empty()) continue;

        auto [minIt, maxIt] = std::minmax_element(data[i].values.begin(), data[i].values.end());
        seriesMin[i] = *minIt;
        seriesMax[i] = *maxIt;

        float range = seriesMax[i] - seriesMin[i];
        if (range < 0.01f) {
            float center = (seriesMin[i] + seriesMax[i]) / 2.0f;
            float padding = std::max(center * 0.1f, 1.0f);
            seriesMin[i] = center - padding;
            seriesMax[i] = center + padding;
        } else {
            float padding = range * 0.1f;
            seriesMin[i] -= padding;
            seriesMax[i] += padding;
        }

        if (*minIt >= 0) seriesMin[i] = std::max(0.0f, seriesMin[i]);
    }

    // Table has n+1 columns: first column for Y-axis labels, then n columns for plots
    // Table has n+1 rows: n rows for plots, last row for X-axis labels
    const int numCols = n + 1;
    const int numRows = n + 1;
    const float labelColWidth = 20.0f;
    // const float labelRowHeight = 50.0f;

    if (ImGui::BeginTable("SPLOM_TABLE", numCols, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInner | ImGuiTableFlags_NoClip)) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        // Account for scrollbar width to prevent overlap
        float scrollbarWidth = ImGui::GetStyle().ScrollbarSize;
        float availableWidth = avail.x - scrollbarWidth;
        float cellSize = (availableWidth - labelColWidth) / n;

        // Setup column widths
        ImGui::TableSetupColumn("YLabels", ImGuiTableColumnFlags_WidthFixed, labelColWidth);
        for (int col = 0; col < n; ++col) {
            ImGui::TableSetupColumn(("Col" + std::to_string(col)).c_str(), ImGuiTableColumnFlags_WidthFixed, cellSize);
        }

        for (int row = 0; row < numRows; ++row) {
            ImGui::TableNextRow();

            // Last row is for X-axis labels
            if (row == n) {
                ImGui::TableSetColumnIndex(0); // Empty corner cell
                for (int col = 0; col < n; ++col) {
                    ImGui::TableSetColumnIndex(col + 1);
                    const std::string& label = spec.series.labels[col];
                    
                    // Use smaller font and wrap text
                    float wrapWidth = cellSize - 4.0f;
                    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + wrapWidth);
                    ImGui::SetWindowFontScale(0.85f);
                    
                    ImVec2 textSize = ImGui::CalcTextSize(label.c_str(), nullptr, false, wrapWidth);
                    float offsetX = (cellSize - std::min(textSize.x, wrapWidth)) * 0.5f;
                    if (offsetX > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
                    
                    ImGui::TextWrapped("%s", label.c_str());
                    ImGui::SetWindowFontScale(1.0f);
                    ImGui::PopTextWrapPos();
                }
                continue;
            }

            // First column is for Y-axis labels (vertical text with wrapping)
            ImGui::TableSetColumnIndex(0);
            {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImFont* font = ImGui::GetFont();
                float fontSize = ImGui::GetFontSize() * 0.8f;
                
                ImVec2 cellPos = ImGui::GetCursorScreenPos();
                float cellCenterX = cellPos.x + labelColWidth * 0.5f;
                float cellCenterY = cellPos.y + cellSize * 0.5f;
                
                const std::string& label = spec.series.labels[row];
                float maxTextHeight = cellSize * 0.85f; // Max height for vertical text
                
                // Split label into lines that fit within maxTextHeight
                std::vector<std::string> lines;
                std::string currentLine;
                std::string word;
                
                for (size_t i = 0; i <= label.size(); ++i) {
                    char c = (i < label.size()) ? label[i] : ' ';
                    if (c == ' ' || i == label.size()) {
                        if (!word.empty()) {
                            std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
                            ImVec2 testSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, testLine.c_str());
                            if (testSize.x > maxTextHeight && !currentLine.empty()) {
                                lines.push_back(currentLine);
                                currentLine = word;
                            } else {
                                currentLine = testLine;
                            }
                            word.clear();
                        }
                    } else {
                        word += c;
                    }
                }
                if (!currentLine.empty()) {
                    lines.push_back(currentLine);
                }
                
                // Draw each line as vertical text (left to right order)
                float lineHeight = fontSize + 2.0f;
                float totalWidth = lines.size() * lineHeight;
                float startX = cellCenterX - totalWidth * 0.5f + lineHeight * 0.5f - 8.0f;
                
                drawList->PushClipRectFullScreen();
                for (size_t lineIdx = 0; lineIdx < lines.size(); ++lineIdx) {
                    const std::string& line = lines[lineIdx];
                    ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, line.c_str());
                    
                    float posX = startX + lineIdx * lineHeight;
                    float posY = cellCenterY + textSize.x * 0.5f;
                    
                    AddTextRotated(drawList, font, fontSize, posX, posY,
                                   IM_COL32(255, 255, 255, 255), line.c_str(), -IM_PI / 2.0f);
                }
                drawList->PopClipRect();
                
                // Reserve space for the row
                ImGui::Dummy(ImVec2(labelColWidth, cellSize));
            }

            // Plot cells
            for (int col = 0; col < n; ++col) {
                ImGui::TableSetColumnIndex(col + 1);
                std::string id = "##sp_" + std::to_string(row) + "_" + std::to_string(col);

                if (ImPlot::BeginPlot(id.c_str(), ImVec2(cellSize, cellSize))) {
                    ImPlot::PushStyleVar(ImPlotStyleVar_FitPadding, ImVec2(0.2f, 0.2f));
                    ImPlot::PushStyleColor(ImPlotCol_FrameBg, ImVec4(0,0,0,0));

                    // All plots have no tick labels - labels are in dedicated row/column
                    ImPlot::SetupAxes(
                        nullptr, nullptr,
                        ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoHighlight,
                        ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoHighlight
                    );

                    ImPlot::SetupAxisLimits(ImAxis_X1, seriesMin[col], seriesMax[col], ImPlotCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1, seriesMin[row], seriesMax[row], ImPlotCond_Always);

                    if (row == col) {
                        // TODO: draw histogram in diagonals.
                        ImGui::Text("%s", spec.series.labels[row].c_str());
                        ImPlot::EndPlot();
                        ImPlot::PopStyleColor();
                        ImPlot::PopStyleVar();
                        continue;
                    }

                    // Ensure highlighted points vector is sized correctly
                    if (s_HighlightedPoints.size() != labels.size()) {
                        s_HighlightedPoints.resize(labels.size(), false);
                    }

                    // Draw scatter points with highlighting
                    for (size_t i = 0; i < labels.size(); ++i) {
                        if (s_HasSelection && s_HighlightedPoints[i]) {
                            // Highlighted points: bright orange
                            ImPlot::PushStyleColor(ImPlotCol_MarkerFill, ImVec4(1.0f, 0.6f, 0.1f, 1.0f));
                            ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, ImVec4(1.0f, 0.8f, 0.3f, 1.0f));
                        } else if (s_HasSelection) {
                            // Non-highlighted points when selection exists: dimmed
                            ImPlot::PushStyleColor(ImPlotCol_MarkerFill, ImVec4(0.3f, 0.5f, 0.7f, 0.3f));
                            ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, ImVec4(0.3f, 0.5f, 0.7f, 0.3f));
                        } else {
                            // Default color when no selection (more transparent)
                            ImPlot::PushStyleColor(ImPlotCol_MarkerFill, ImVec4(0.3f, 0.7f, 1.0f, 0.5f));
                            ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, ImVec4(0.3f, 0.7f, 1.0f, 0.5f));
                        }
                        
                        ImPlot::PlotScatter(
                            ("##" + labels[i]).c_str(),
                            &data[col].values[i],
                            &data[row].values[i],
                            1
                        );
                        
                        ImPlot::PopStyleColor(2);
                    }

                    // Handle drag selection
                    if (ImPlot::IsPlotHovered()) {
                        ImVec2 mousePix = ImGui::GetMousePos();
                        ImPlotPoint mousePlot = ImPlot::GetPlotMousePos();
                        
                        // Start dragging on mouse down
                        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                            s_IsDragging = true;
                            s_DragStartX = static_cast<float>(mousePlot.x);
                            s_DragStartY = static_cast<float>(mousePlot.y);
                            s_DragEndX = s_DragStartX;
                            s_DragEndY = s_DragStartY;
                            s_DragPlotRow = row;
                            s_DragPlotCol = col;
                        }
                    }
                    
                    // Continue dragging even if mouse moves outside plot
                    if (s_IsDragging && s_DragPlotRow == row && s_DragPlotCol == col) {
                        ImPlotPoint mousePlot = ImPlot::GetPlotMousePos();
                        s_DragEndX = static_cast<float>(mousePlot.x);
                        s_DragEndY = static_cast<float>(mousePlot.y);
                        
                        // Draw selection rectangle
                        ImDrawList* drawList = ImPlot::GetPlotDrawList();
                        ImVec2 p1 = ImPlot::PlotToPixels(s_DragStartX, s_DragStartY);
                        ImVec2 p2 = ImPlot::PlotToPixels(s_DragEndX, s_DragEndY);
                        drawList->AddRectFilled(p1, p2, IM_COL32(255, 165, 0, 50));
                        drawList->AddRect(p1, p2, IM_COL32(255, 165, 0, 200), 0.0f, 0, 2.0f);
                        
                        // Release drag on mouse up
                        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                            s_IsDragging = false;
                            
                            // Calculate selection bounds
                            float minX = std::min(s_DragStartX, s_DragEndX);
                            float maxX = std::max(s_DragStartX, s_DragEndX);
                            float minY = std::min(s_DragStartY, s_DragEndY);
                            float maxY = std::max(s_DragStartY, s_DragEndY);
                            
                            // Only create selection if drag was significant
                            float dragDist = std::abs(maxX - minX) + std::abs(maxY - minY);
                            if (dragDist > 0.01f) {
                                s_HasSelection = true;
                                
                                // Update highlighted points based on selection rectangle
                                for (size_t i = 0; i < labels.size(); ++i) {
                                    float px = data[col].values[i];
                                    float py = data[row].values[i];
                                    s_HighlightedPoints[i] = (px >= minX && px <= maxX && py >= minY && py <= maxY);
                                }
                            }
                            // If drag was not significant (just a click), do nothing - keep current selection
                        }
                    }
                    
                    // Draw selection rectangle on other plots if selection exists
                    if (s_HasSelection && (s_DragPlotRow != row || s_DragPlotCol != col)) {
                        // We show highlighting on all plots via the point colors above
                    }

                    // Tooltip on hover (only when not dragging)
                    if (!s_IsDragging && ImPlot::IsPlotHovered()) {

                        float minDist = FLT_MAX;
                        int closestIdx = -1;

                        for (size_t i = 0; i < labels.size(); ++i) {
                            ImVec2 dataPix = ImPlot::PlotToPixels(data[col].values[i], data[row].values[i]);
                            ImVec2 mousePix = ImPlot::PlotToPixels(ImPlot::GetPlotMousePos());
                            float d = static_cast<float>(pow(mousePix.x - dataPix.x, 2) + pow(mousePix.y - dataPix.y, 2));
                            if (d < minDist) {
                                minDist = d;
                                closestIdx = (int)i;
                            }
                        }

                        if (closestIdx >= 0 && minDist < 400.0f) {
                            ImGui::BeginTooltip();
                            ImGui::Text("%s", labels[closestIdx].c_str());
                            if (!data[col].unit.empty()) {
                                ImGui::Text("X: %.2f %s", data[col].values[closestIdx], data[col].unit.c_str());
                            } else {
                                ImGui::Text("X: %.2f", data[col].values[closestIdx]);
                            }
                            if (!data[row].unit.empty()) {
                                ImGui::Text("Y: %.2f %s", data[row].values[closestIdx], data[row].unit.c_str());
                            } else {
                                ImGui::Text("Y: %.2f", data[row].values[closestIdx]);
                            }
                            ImGui::EndTooltip();
                        }
                    }

                    ImPlot::EndPlot();
                    ImPlot::PopStyleColor();
                    ImPlot::PopStyleVar();
                }
            }
        }
        ImGui::EndTable();
    }
    
    // Clear selection on right-click anywhere in the SPLOM area
    if (s_HasSelection && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        s_HasSelection = false;
        s_IsDragging = false;
        std::fill(s_HighlightedPoints.begin(), s_HighlightedPoints.end(), false);
    }
}

void GraphRenderer::RenderRadar(const GraphSpec& spec,
                                const std::vector<std::string>& labels,
                                const std::vector<PlotSeries>& data)
{
    // Initialize features from data if empty or feature count changed
    if ((g_ActiveFeatures.empty() && g_AvailableFeatures.empty()) || 
        (g_ActiveFeatures.size() + g_AvailableFeatures.size()) != data.size()) {
        g_ActiveFeatures.clear();
        g_AvailableFeatures.clear();
        for (size_t i = 0; i < data.size(); ++i) {
            RadarFeature feature;
            feature.id = static_cast<int>(i);
            feature.label = (i < spec.series.labels.size()) ? spec.series.labels[i] : ("Feature " + std::to_string(i));
            feature.series = data[i];
            g_ActiveFeatures.push_back(feature);
        }
    } else {
        // Update series data for all features (countries may have changed)
        for (auto& feature : g_ActiveFeatures) {
            if (feature.id >= 0 && feature.id < (int)data.size()) {
                feature.series = data[feature.id];
            }
        }
        for (auto& feature : g_AvailableFeatures) {
            if (feature.id >= 0 && feature.id < (int)data.size()) {
                feature.series = data[feature.id];
            }
        }
    }

    // Layout: Feature list on left, Radar chart on right
    ImGui::BeginGroup();
    
    // ================ FEATURE LIST PANEL ================
    ImGui::BeginChild("FeaturePanel", ImVec2(150, 400), true);
    ImGui::TextUnformatted("Features");
    ImGui::Separator();
    
    // Show only available (inactive) features
    for (size_t i = 0; i < g_AvailableFeatures.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        ImGui::Selectable(g_AvailableFeatures[i].label.c_str(), false);
        
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            int idx = static_cast<int>(i);
            ImGui::SetDragDropPayload("RADAR_FEATURE", &idx, sizeof(int));
            ImGui::Text("Add: %s", g_AvailableFeatures[i].label.c_str());
            ImGui::EndDragDropSource();
        }
        ImGui::PopID();
    }
    
    // Drop target for removing features (entire panel)
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RADAR_REMOVE")) {
            int idx = *(int*)payload->Data;
            if (idx >= 0 && idx < (int)g_ActiveFeatures.size()) {
                g_AvailableFeatures.push_back(g_ActiveFeatures[idx]);
                g_ActiveFeatures.erase(g_ActiveFeatures.begin() + idx);
            }
        }
        ImGui::EndDragDropTarget();
    }
    
    ImGui::EndChild();
    
    // Also make the child window itself a drop target
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RADAR_REMOVE")) {
            int idx = *(int*)payload->Data;
            if (idx >= 0 && idx < (int)g_ActiveFeatures.size()) {
                g_AvailableFeatures.push_back(g_ActiveFeatures[idx]);
                g_ActiveFeatures.erase(g_ActiveFeatures.begin() + idx);
            }
        }
        ImGui::EndDragDropTarget();
    }
    
    ImGui::SameLine();
    
    // ================ RADAR CHART ================
    ImGui::BeginGroup();
    
    if (!ImPlot::BeginPlot(spec.title.c_str(), ImVec2(-1, 400),
        ImPlotFlags_NoLegend | ImPlotFlags_NoMenus | ImPlotFlags_NoFrame)) {
        ImGui::EndGroup();
        ImGui::EndGroup();
        return;
    }

    // Setup must happen immediately after BeginPlot, before any locking functions
    ImPlot::SetupAxes(nullptr, nullptr,
        ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoHighlight, ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoHighlight);
    ImPlot::SetupAxisLimits(ImAxis_X1, -1, 1, ImPlotCond_Always);
    ImPlot::SetupAxisLimits(ImAxis_Y1, -1, 1, ImPlotCond_Always);

    // ---------------- Drag & Drop Target ----------------

    if (ImPlot::BeginDragDropTargetPlot()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RADAR_FEATURE")) {
            int src = *(int*)payload->Data;
            g_ActiveFeatures.push_back(g_AvailableFeatures[src]);
            g_AvailableFeatures.erase(g_AvailableFeatures.begin() + src);
        }
        ImPlot::EndDragDropTarget();
    }

    const int axisCount = (int)g_ActiveFeatures.size();
    if (axisCount == 0) { ImPlot::EndPlot(); return; }

    ImDrawList* draw = ImPlot::GetPlotDrawList();
    ImVec2 center = ImPlot::PlotToPixels({0, 0});
    ImVec2 plotSize = ImPlot::GetPlotSize();
    float radius = std::min(plotSize.x, plotSize.y) * 0.4f;

    // ---------------- Normalization ----------------

    std::vector<float> maxVals(axisCount, 0.0f);
    for (int a = 0; a < axisCount; ++a)
        for (float v : g_ActiveFeatures[a].series.values)
            maxVals[a] = std::max(maxVals[a], v);

    // ---------------- Grid & Axes ----------------

    for (int a = 0; a < axisCount; ++a) {
        float ang = 2 * IM_PI * a / axisCount - IM_PI / 2;

        ImVec2 p = { center.x + cosf(ang) * radius,
                     center.y + sinf(ang) * radius };

        draw->AddLine(center, p, IM_COL32(120,120,120,200), 1.0f);

        ImVec2 labelPos = { center.x + cosf(ang) * (radius + 14),
                            center.y + sinf(ang) * (radius + 14) };

        // Calculate text size for offset adjustment
        const char* labelText = g_ActiveFeatures[a].label.c_str();
        ImVec2 textSize = ImGui::CalcTextSize(labelText);

        // Offset labels based on their position relative to the chart center
        // Left side labels (cos < 0) need to be shifted left by their width
        // Right side labels stay as-is, center labels are centered
        float cosAng = cosf(ang);
        if (cosAng < -0.1f) {
            // Left side: shift left by text width
            labelPos.x -= textSize.x;
        } else if (cosAng > 0.1f) {
            // Right side: no horizontal adjustment needed
        } else {
            // Top/bottom center: center the text horizontally
            labelPos.x -= textSize.x * 0.5f;
        }

        // Adjust vertical position for top/bottom labels
        float sinAng = sinf(ang);
        if (sinAng < -0.1f) {
            // Top labels: shift up slightly
            labelPos.y -= textSize.y * 0.5f;
        } else if (sinAng > 0.1f) {
            // Bottom labels: shift down slightly
            labelPos.y += textSize.y * 0.25f;
        }

        draw->AddText(labelPos, IM_COL32_WHITE, labelText);

        // -------- Axis Drag Source (remove) --------

        ImGui::SetCursorScreenPos(labelPos);
        ImGui::InvisibleButton(("##axis" + std::to_string(a)).c_str(), ImVec2(90,18));

        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("RADAR_REMOVE", &a, sizeof(int));
            ImGui::Text("%s", g_ActiveFeatures[a].label.c_str());
            ImGui::EndDragDropSource();
        }
    }

    // ---------------- Rings ----------------

    for (int ring = 1; ring <= 4; ++ring) {
        float rr = radius * ring / 4.0f;
        for (int a = 0; a < axisCount; ++a) {
            float a0 = 2 * IM_PI * a / axisCount - IM_PI / 2;
            float a1 = 2 * IM_PI * (a + 1) / axisCount - IM_PI / 2;

            draw->AddLine(
                { center.x + cosf(a0) * rr, center.y + sinf(a0) * rr },
                { center.x + cosf(a1) * rr, center.y + sinf(a1) * rr },
                IM_COL32(80,80,80,150), 1.0f);
        }
    }

    // ---------------- Polygons ----------------

    const size_t countryCount = g_ActiveFeatures[0].series.values.size();

    for (size_t c = 0; c < countryCount; ++c) {
        ImVec4 colVec = ImColor::HSV((float)c / countryCount, 0.6f, 0.9f);
        ImU32 col = ImColor(colVec);
        ImU32 fillCol = IM_COL32((int)(colVec.x * 255), (int)(colVec.y * 255), (int)(colVec.z * 255), 50);
        std::vector<ImVec2> poly;

        for (int a = 0; a < axisCount; ++a) {
            float ang = 2 * IM_PI * a / axisCount - IM_PI / 2;
            float norm = maxVals[a] > 0
                ? g_ActiveFeatures[a].series.values[c] / maxVals[a]
                : 0.0f;

            poly.push_back({ center.x + cosf(ang) * (norm * radius),
                             center.y + sinf(ang) * (norm * radius) });
        }

        // Draw filled polygon using triangle fan from center (works for concave polygons)
        if (poly.size() >= 3) {
            for (int i = 0; i < axisCount; ++i) {
                int next = (i + 1) % axisCount;
                draw->AddTriangleFilled(center, poly[i], poly[next], fillCol);
            }
        }

        // Draw outline and points
        for (int i = 0; i < axisCount; ++i) {
            draw->AddLine(poly[i], poly[(i+1)%axisCount], col, 2.0f);
            draw->AddCircleFilled(poly[i], 3.5f, col);
        }
    }

    // ---------------- Hover Tooltip ----------------

    if (ImPlot::IsPlotHovered()) {
        ImVec2 mouse = ImGui::GetMousePos();
        float bestDist = FLT_MAX;
        int bestCountry = -1;

        for (int c = 0; c < countryCount; ++c) {
            for (int a = 0; a < axisCount; ++a) {
                float ang = 2 * IM_PI * a / axisCount - IM_PI / 2;
                float norm = maxVals[a] > 0
                    ? g_ActiveFeatures[a].series.values[c] / maxVals[a]
                    : 0.0f;

                ImVec2 p = { center.x + cosf(ang) * (norm * radius),
                             center.y + sinf(ang) * (norm * radius) };

                float d = (mouse.x-p.x)*(mouse.x-p.x) + (mouse.y-p.y)*(mouse.y-p.y);
                if (d < bestDist) { bestDist = d; bestCountry = c; }
            }
        }

        if (bestCountry >= 0 && bestDist < 64.0f) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", labels[bestCountry].c_str());
            ImGui::Separator();
            for (int a = 0; a < axisCount; ++a)
                ImGui::Text("%s: %.2f",
                    g_ActiveFeatures[a].label.c_str(),
                    g_ActiveFeatures[a].series.values[bestCountry]);
            ImGui::EndTooltip();
        }
    }

    ImPlot::EndPlot();
    
    ImGui::EndGroup();  // End radar chart group
    ImGui::EndGroup();  // End main layout group
}


void GraphRenderer::AddTextRotated(void* drawListPtr, void* fontPtr, float fontSize,
                                   float posX, float posY, unsigned int col,
                                   const char* text, float angle)
{
    ImDrawList* draw_list = (ImDrawList*)drawListPtr;
    ImFont* font = (ImFont*)fontPtr;

    if (!text || !text[0]) return;

    const int vtx_start = draw_list->VtxBuffer.Size;
    draw_list->AddText(font, fontSize, ImVec2(posX, posY), col, text);

    const float s = sinf(angle);
    const float c = cosf(angle);

    for (int i = vtx_start; i < draw_list->VtxBuffer.Size; i++) {
        ImVec2& p = draw_list->VtxBuffer[i].pos;
        float x = p.x - posX;
        float y = p.y - posY;
        p.x = posX + (x * c - y * s);
        p.y = posY + (x * s + y * c);
    }
}

void GraphRenderer::DrawRotatedLabels(const std::vector<std::string>& labels)
{
    ImDrawList* drawList = ImPlot::GetPlotDrawList();
    ImPlotRect plot = ImPlot::GetPlotLimits();
    ImFont* font = ImGui::GetFont();
    float fontSize = ImGui::GetFontSize();

    const float angle = -IM_PI / 4.0f;
    const float yOffset = 30.0f;

    for (size_t i = 0; i < labels.size(); ++i) {
        ImPlotPoint pPlot((double)i, plot.Y.Min);
        ImVec2 pPix = ImPlot::PlotToPixels(pPlot);

        ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, labels[i].c_str());

        // Alignment
        float diagonal = textSize.x * cosf(angle);
        pPix.x += textSize.x * sinf(angle);
        pPix.y += yOffset + diagonal;

        AddTextRotated(drawList, font, fontSize, pPix.x, pPix.y,
                       IM_COL32(255,255,255,255),
                       labels[i].c_str(), angle);
    }
}

#include "GraphRenderer.hpp"

#include <imgui.h>
#include <implot.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <iostream>

// Static member definitions
std::vector<GraphRenderer::RadarFeature> GraphRenderer::g_AvailableFeatures;
std::vector<GraphRenderer::RadarFeature> GraphRenderer::g_ActiveFeatures;

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
                           std::vector<PlotSeries>& data,
                           const std::string& highlight)
{
    if (itemLabels.empty() || data.empty()) {
        ImGui::TextDisabled("No data selected to render.");
        return;
    }

    switch (spec.type) {
        case GraphType::SPLOM:
            RenderSPLOM(spec, itemLabels, data, highlight);
            return;
        case GraphType::Radar:
            RenderRadar(spec, itemLabels, data, highlight);
            return;
        case GraphType::TreeMap:
            RenderTreeMap(spec, itemLabels, data, highlight);
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
            RenderBar(spec, itemLabels, data, highlight);
        } else if (spec.type == GraphType::Scatter) {
            RenderScatter(spec, itemLabels, data, highlight);
        }

        ImPlot::EndPlot();
    }

    ImPlot::PopStyleColor();
    ImPlot::PopStyleVar(2);
}

// =========================================================
// TREEMAP IMPLEMENTATION (SQUARIFIED)
// =========================================================

void GraphRenderer::RenderTreeMap(const GraphSpec& spec,
                                  const std::vector<std::string>& labels,
                                  const std::vector<PlotSeries>& data,
                                  const std::string& highlight)
{
    if (data.empty()) return;

    ImGui::TextUnformatted(spec.title.c_str());

    // 1. Setup Canvas
    ImVec2 avail = ImGui::GetContentRegionAvail();
    avail.y = std::max(avail.y, 400.0f); // Minimum height
    ImVec2 p0 = ImGui::GetCursorScreenPos();
    ImVec2 p1 = ImVec2(p0.x + avail.x, p0.y + avail.y);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(p0, p1, IM_COL32(30, 30, 30, 255));

    // 2. Prepare Data
    std::vector<TreemapNode> nodes;
    float totalValue = 0.0f;
    
    for (size_t i = 0; i < labels.size(); ++i) {
        float val = data[0].values[i];
        if (val > 0) {
            TreemapNode node;
            node.label = labels[i];
            node.value = val;
            node.originalIndex = (int)i;
            nodes.push_back(node);
            totalValue += val;
        }
    }

    if (totalValue <= 0) {
        ImGui::Dummy(avail);
        return;
    }

    // Sort descending (critical for squarify)
    std::sort(nodes.begin(), nodes.end(), [](const TreemapNode& a, const TreemapNode& b) {
        return a.value > b.value;
    });

    // Calculate pixel area for each node
    float totalArea = avail.x * avail.y;
    for (auto& node : nodes) {
        node.area = (node.value / totalValue) * totalArea;
    }

    // 3. Squarified Algorithm
    Rect remainingRect = { p0.x, p0.y, avail.x, avail.y };
    std::vector<TreemapNode*> currentRow;
    
    // We process nodes one by one
    for (size_t i = 0; i < nodes.size(); ++i) {
        TreemapNode* node = &nodes[i];
        
        if (currentRow.empty()) {
            currentRow.push_back(node);
        } else {
            // Check if adding this node improves aspect ratio
            float side = remainingRect.shortestSide();
            
            // Current worst ratio
            float currentWorst = WorstAspectRatio(currentRow, side);
            
            // Worst ratio if we add the new node
            std::vector<TreemapNode*> proposedRow = currentRow;
            proposedRow.push_back(node);
            float proposedWorst = WorstAspectRatio(proposedRow, side);
            
            if (currentWorst >= proposedWorst) {
                // It improved (or stayed same), so add it
                currentRow.push_back(node);
            } else {
                // It got worse, so finalize the current row
                // "Layout" determines if we slice vertically or horizontally based on rect shape
                LayoutRow(currentRow, remainingRect, remainingRect.w < remainingRect.h);
                currentRow.clear();
                currentRow.push_back(node);
            }
        }
    }

    // Finalize any remaining nodes
    if (!currentRow.empty()) {
        LayoutRow(currentRow, remainingRect, remainingRect.w < remainingRect.h);
    }

    // 4. Render
    for (const auto& node : nodes) {
        DrawTreemapNode(drawList, node, highlight, spec, data[0]);
    }
    
    ImGui::Dummy(avail);
}

float GraphRenderer::WorstAspectRatio(const std::vector<TreemapNode*>& row, float sideLength) {
    if (row.empty() || sideLength <= 0.0f) return FLT_MAX;
    
    float totalArea = 0.0f;
    float minArea = FLT_MAX;
    float maxArea = 0.0f;
    
    for (auto* node : row) {
        totalArea += node->area;
        if (node->area < minArea) minArea = node->area;
        if (node->area > maxArea) maxArea = node->area;
    }
    
    if (totalArea == 0.0f) return FLT_MAX;
    
    float s2 = sideLength * sideLength;
    float a2 = totalArea * totalArea;
    
    // Max of (w/h) or (h/w)
    // Based on squarify formula
    return std::max( (s2 * maxArea) / a2, a2 / (s2 * minArea) );
}

void GraphRenderer::LayoutRow(std::vector<TreemapNode*>& row, Rect& container, bool vertical) {
    if (row.empty()) return;
    
    float rowArea = 0.0f;
    for (auto* node : row) rowArea += node->area;
    
    if (vertical) {
        // Container is taller than wide (or we chose to stack against top edge)
        // Actually 'vertical' param here means "split horizontal space", i.e. stack along width?
        // Let's stick to the convention: sideLength is the shortest side.
        // If w < h, shortest is width. We create a horizontal bar of height h_row.
        
        float rowHeight = rowArea / container.w;
        float currentX = container.x;
        
        for (auto* node : row) {
            float nodeWidth = node->area / rowHeight;
            node->x = currentX;
            node->y = container.y;
            node->w = nodeWidth;
            node->h = rowHeight;
            currentX += nodeWidth;
        }
        
        // Update container
        container.y += rowHeight;
        container.h -= rowHeight;
        
    } else {
        // Container is wider than tall. Shortest side is height.
        // We create a vertical bar of width w_row.
        
        float rowWidth = rowArea / container.h;
        float currentY = container.y;
        
        for (auto* node : row) {
            float nodeHeight = node->area / rowWidth;
            node->x = container.x;
            node->y = currentY;
            node->w = rowWidth;
            node->h = nodeHeight;
            currentY += nodeHeight;
        }
        
        // Update container
        container.x += rowWidth;
        container.w -= rowWidth;
    }
}

void GraphRenderer::DrawTreemapNode(void* drawListPtr, const TreemapNode& node, 
                                    const std::string& highlight, const GraphSpec& spec, const PlotSeries& dataSeries)
{
    ImDrawList* drawList = (ImDrawList*)drawListPtr;
    
    ImVec2 pMin(node.x, node.y);
    ImVec2 pMax(node.x + node.w, node.y + node.h);
    
    // Padding
    pMin.x += 1; pMin.y += 1;
    pMax.x -= 1; pMax.y -= 1;
    
    if (pMax.x <= pMin.x || pMax.y <= pMin.y) return;

    // Color logic
    ImU32 col;
    bool isHighlighted = (!highlight.empty() && node.label == highlight);
    
    if (isHighlighted) {
         col = IM_COL32(255, 200, 50, 255); // Bright Yellow/Orange
    } else {
         // Hash string to get consistent random color, or gradient
         // Simple gradient based on area size often looks good in treemaps
         // Or just hash the name for distinct colors
         size_t hash = std::hash<std::string>{}(node.label);
         float hue = (hash % 100) / 100.0f;
         col = ImColor::HSV(hue, 0.6f, 0.7f);
    }
    
    drawList->AddRectFilled(pMin, pMax, col);
    
    if (isHighlighted) {
         drawList->AddRect(pMin, pMax, IM_COL32(255, 255, 255, 255), 0.0f, 0, 3.0f);
    }

    // Hover
    if (ImGui::IsMouseHoveringRect(pMin, pMax)) {
         drawList->AddRect(pMin, pMax, IM_COL32(255, 255, 255, 150), 0.0f, 0, 2.0f);
         ImGui::BeginTooltip();
         ImGui::Text("%s", node.label.c_str());
         ImGui::Text("%s: %.2f %s", spec.series.labels[0].c_str(), node.value, dataSeries.unit.c_str());
         ImGui::EndTooltip();
    }

    // Label if fits
    if (node.w > 30 && node.h > 15) {
        std::string label = node.label;
        ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
        
        // Truncate if too long
        if (textSize.x > node.w - 4) {
            // Very basic truncation
            int chars = (int)((node.w - 4) / (textSize.x / label.length()));
            if (chars > 2) label = label.substr(0, chars - 2) + "..";
            else label = "";
            textSize = ImGui::CalcTextSize(label.c_str());
        }
        
        if (!label.empty() && textSize.y < node.h) {
            ImVec2 textPos = ImVec2(
                pMin.x + (node.w - textSize.x) * 0.5f,
                pMin.y + (node.h - textSize.y) * 0.5f
            );
            drawList->AddText(textPos, IM_COL32_WHITE, label.c_str());
        }
    }
}

// =========================================================
// END TREEMAP
// =========================================================

void GraphRenderer::RenderBar(const GraphSpec& spec,
                              const std::vector<std::string>& labels,
                              const std::vector<PlotSeries>& data,
                              const std::string& highlight)
{
    std::vector<float> x(labels.size());
    for (size_t i = 0; i < x.size(); ++i) x[i] = (float)i;

    for (size_t i = 0; i < data.size(); ++i) {
        const auto& s = data[i];

        float barWidth = 0.8f / data.size();
        float offset = (i - (data.size() - 1) / 2.0f) * barWidth;

        std::vector<float> xOffset(x.size());
        for (size_t j = 0; j < x.size(); ++j) xOffset[j] = x[j] + offset;

        // Standard PlotBars
        ImPlot::PlotBars(spec.series.labels[i].c_str(), xOffset.data(), s.values.data(), (int)xOffset.size(), barWidth);
        
        // Highlight Overlay
        if (!highlight.empty()) {
            for (size_t k = 0; k < labels.size(); ++k) {
                if (labels[k] == highlight) {
                    double h_x = xOffset[k];
                    double h_y = s.values[k];
                    ImPlot::PushStyleColor(ImPlotCol_Fill, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));
                    ImPlot::PlotBars("##Highlight", &h_x, &h_y, 1, barWidth);
                    ImPlot::PopStyleColor();
                }
            }
        }
    }
    DrawRotatedLabels(labels);
}

void GraphRenderer::RenderScatter(const GraphSpec&,
                                  const std::vector<std::string>& labels,
                                  const std::vector<PlotSeries>& data,
                                  const std::string& highlight)
{
    if (data.size() < 2) {
        ImGui::Text("Scatter plot requires at least 2 data series (X and Y).");
        return;
    }

    ImPlot::PushStyleVar(ImPlotStyleVar_Marker, ImPlotMarker_Cross);

    for (size_t i = 0; i < labels.size(); ++i) {
        bool isHigh = (labels[i] == highlight);
        if (isHigh) {
            ImPlot::PushStyleColor(ImPlotCol_MarkerFill, ImVec4(1, 0.8f, 0, 1)); 
            ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, ImVec4(1, 0.8f, 0, 1)); 
            ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, 8.0f);
        }

        ImPlot::PlotScatter(labels[i].c_str(), &data[0].values[i], &data[1].values[i], 1);
        
        if (isHigh) {
            ImPlot::PopStyleVar();
            ImPlot::PopStyleColor(2);
        }
    }

    ImPlot::PopStyleVar();

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
                                const std::vector<PlotSeries>& data,
                                const std::string& highlight)
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

    const int numCols = n + 1;
    const int numRows = n + 1;
    const float labelColWidth = 20.0f;

    if (ImGui::BeginTable("SPLOM_TABLE", numCols, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInner | ImGuiTableFlags_NoClip)) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float scrollbarWidth = ImGui::GetStyle().ScrollbarSize;
        float availableWidth = avail.x - scrollbarWidth;
        float cellSize = (availableWidth - labelColWidth) / n;

        ImGui::TableSetupColumn("YLabels", ImGuiTableColumnFlags_WidthFixed, labelColWidth);
        for (int col = 0; col < n; ++col) {
            ImGui::TableSetupColumn(("Col" + std::to_string(col)).c_str(), ImGuiTableColumnFlags_WidthFixed, cellSize);
        }

        for (int row = 0; row < numRows; ++row) {
            ImGui::TableNextRow();

            // X-Axis Labels (Bottom Row)
            if (row == n) {
                ImGui::TableSetColumnIndex(0);
                for (int col = 0; col < n; ++col) {
                    ImGui::TableSetColumnIndex(col + 1);
                    const std::string& label = spec.series.labels[col];
                    float wrapWidth = cellSize - 4.0f;
                    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + wrapWidth);
                    ImGui::SetWindowFontScale(0.85f);
                    ImGui::TextWrapped("%s", label.c_str());
                    ImGui::SetWindowFontScale(1.0f);
                    ImGui::PopTextWrapPos();
                }
                continue;
            }

            // Y-Axis Labels
            ImGui::TableSetColumnIndex(0);
            {
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImFont* font = ImGui::GetFont();
                float fontSize = ImGui::GetFontSize() * 0.8f;
                ImVec2 cellPos = ImGui::GetCursorScreenPos();
                float cellCenterX = cellPos.x + labelColWidth * 0.5f;
                float cellCenterY = cellPos.y + cellSize * 0.5f;
                const std::string& label = spec.series.labels[row];
                AddTextRotated(drawList, font, fontSize, cellCenterX - 4, cellCenterY + 10, IM_COL32(255, 255, 255, 255), label.c_str(), -IM_PI / 2.0f);
                ImGui::Dummy(ImVec2(labelColWidth, cellSize));
            }

            for (int col = 0; col < n; ++col) {
                ImGui::TableSetColumnIndex(col + 1);
                std::string id = "##sp_" + std::to_string(row) + "_" + std::to_string(col);

                if (ImPlot::BeginPlot(id.c_str(), ImVec2(cellSize, cellSize))) {
                    ImPlot::PushStyleVar(ImPlotStyleVar_FitPadding, ImVec2(0.2f, 0.2f));
                    ImPlot::PushStyleColor(ImPlotCol_FrameBg, ImVec4(0,0,0,0));
                    ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoHighlight, ImPlotAxisFlags_AutoFit | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoHighlight);
                    ImPlot::SetupAxisLimits(ImAxis_X1, seriesMin[col], seriesMax[col], ImPlotCond_Always);
                    ImPlot::SetupAxisLimits(ImAxis_Y1, seriesMin[row], seriesMax[row], ImPlotCond_Always);

                    if (row == col) {
                        ImGui::Text("%s", spec.series.labels[row].c_str());
                        ImPlot::EndPlot();
                        ImPlot::PopStyleColor();
                        ImPlot::PopStyleVar();
                        continue;
                    }

                    if (s_HighlightedPoints.size() != labels.size()) s_HighlightedPoints.resize(labels.size(), false);

                    for (size_t i = 0; i < labels.size(); ++i) {
                        bool isHoverHighlight = (!highlight.empty() && labels[i] == highlight);
                        bool isDragSelected = s_HasSelection && s_HighlightedPoints[i];

                        if (isHoverHighlight) {
                            ImPlot::PushStyleColor(ImPlotCol_MarkerFill, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
                            ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                            ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, 6.0f);
                        } else if (isDragSelected) {
                            ImPlot::PushStyleColor(ImPlotCol_MarkerFill, ImVec4(1.0f, 0.6f, 0.1f, 1.0f));
                            ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, ImVec4(1.0f, 0.8f, 0.3f, 1.0f));
                        } else if (s_HasSelection) {
                            ImPlot::PushStyleColor(ImPlotCol_MarkerFill, ImVec4(0.3f, 0.5f, 0.7f, 0.2f));
                            ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, ImVec4(0.3f, 0.5f, 0.7f, 0.2f));
                        } else {
                            ImPlot::PushStyleColor(ImPlotCol_MarkerFill, ImVec4(0.3f, 0.7f, 1.0f, 0.5f));
                            ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, ImVec4(0.3f, 0.7f, 1.0f, 0.5f));
                        }
                        
                        ImPlot::PlotScatter(("##" + labels[i]).c_str(), &data[col].values[i], &data[row].values[i], 1);
                        
                        ImPlot::PopStyleColor(2);
                        if (isHoverHighlight) ImPlot::PopStyleVar();
                    }

                    if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                        s_IsDragging = true;
                        ImPlotPoint mousePlot = ImPlot::GetPlotMousePos();
                        s_DragStartX = (float)mousePlot.x; s_DragStartY = (float)mousePlot.y;
                        s_DragEndX = s_DragStartX; s_DragEndY = s_DragStartY;
                        s_DragPlotRow = row; s_DragPlotCol = col;
                    }

                    if (s_IsDragging && s_DragPlotRow == row && s_DragPlotCol == col) {
                        ImPlotPoint mousePlot = ImPlot::GetPlotMousePos();
                        s_DragEndX = (float)mousePlot.x; s_DragEndY = (float)mousePlot.y;
                        ImDrawList* drawList = ImPlot::GetPlotDrawList();
                        ImVec2 p1 = ImPlot::PlotToPixels(s_DragStartX, s_DragStartY);
                        ImVec2 p2 = ImPlot::PlotToPixels(s_DragEndX, s_DragEndY);
                        drawList->AddRectFilled(p1, p2, IM_COL32(255, 165, 0, 50));
                        drawList->AddRect(p1, p2, IM_COL32(255, 165, 0, 200), 0.0f, 0, 2.0f);
                        
                        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
                            s_IsDragging = false;
                            float minX = std::min(s_DragStartX, s_DragEndX);
                            float maxX = std::max(s_DragStartX, s_DragEndX);
                            float minY = std::min(s_DragStartY, s_DragEndY);
                            float maxY = std::max(s_DragStartY, s_DragEndY);
                            if (std::abs(maxX - minX) + std::abs(maxY - minY) > 0.01f) {
                                s_HasSelection = true;
                                for (size_t i = 0; i < labels.size(); ++i) {
                                    float px = data[col].values[i];
                                    float py = data[row].values[i];
                                    s_HighlightedPoints[i] = (px >= minX && px <= maxX && py >= minY && py <= maxY);
                                }
                            }
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
    
    if (s_HasSelection && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        s_HasSelection = false;
        s_IsDragging = false;
        std::fill(s_HighlightedPoints.begin(), s_HighlightedPoints.end(), false);
    }
}

void GraphRenderer::RenderRadar(const GraphSpec& spec,
                                const std::vector<std::string>& labels,
                                const std::vector<PlotSeries>& data,
                                const std::string& highlight)
{
    if ((g_ActiveFeatures.empty() && g_AvailableFeatures.empty()) || 
        (g_ActiveFeatures.size() + g_AvailableFeatures.size()) != data.size()) {
        g_ActiveFeatures.clear();
        g_AvailableFeatures.clear();
        for (size_t i = 0; i < data.size(); ++i) {
            RadarFeature feature;
            feature.id = (int)i;
            feature.label = (i < spec.series.labels.size()) ? spec.series.labels[i] : ("Feature " + std::to_string(i));
            feature.series = data[i];
            g_ActiveFeatures.push_back(feature);
        }
    } else {
        for (auto& feature : g_ActiveFeatures) if (feature.id >= 0 && feature.id < (int)data.size()) feature.series = data[feature.id];
        for (auto& feature : g_AvailableFeatures) if (feature.id >= 0 && feature.id < (int)data.size()) feature.series = data[feature.id];
    }

    ImGui::BeginGroup();
    ImGui::BeginChild("FeaturePanel", ImVec2(150, 400), true);
    ImGui::TextUnformatted("Features");
    ImGui::Separator();
    for (size_t i = 0; i < g_AvailableFeatures.size(); ++i) {
        ImGui::PushID((int)i);
        ImGui::Selectable(g_AvailableFeatures[i].label.c_str(), false);
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            int idx = (int)i;
            ImGui::SetDragDropPayload("RADAR_FEATURE", &idx, sizeof(int));
            ImGui::Text("Add: %s", g_AvailableFeatures[i].label.c_str());
            ImGui::EndDragDropSource();
        }
        ImGui::PopID();
    }
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
    ImGui::BeginGroup();

    if (!ImPlot::BeginPlot(spec.title.c_str(), ImVec2(-1, 400), ImPlotFlags_NoLegend | ImPlotFlags_NoMenus | ImPlotFlags_NoFrame)) {
        ImGui::EndGroup(); ImGui::EndGroup(); return;
    }
    ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
    ImPlot::SetupAxisLimits(ImAxis_X1, -1, 1, ImPlotCond_Always);
    ImPlot::SetupAxisLimits(ImAxis_Y1, -1, 1, ImPlotCond_Always);
    
    if (ImPlot::BeginDragDropTargetPlot()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RADAR_FEATURE")) {
            int src = *(int*)payload->Data;
            g_ActiveFeatures.push_back(g_AvailableFeatures[src]);
            g_AvailableFeatures.erase(g_AvailableFeatures.begin() + src);
        }
        ImPlot::EndDragDropTarget();
    }

    const int axisCount = (int)g_ActiveFeatures.size();
    if (axisCount == 0) { ImPlot::EndPlot(); ImGui::EndGroup(); ImGui::EndGroup(); return; }

    ImDrawList* draw = ImPlot::GetPlotDrawList();
    ImVec2 center = ImPlot::PlotToPixels({0, 0});
    ImVec2 plotSize = ImPlot::GetPlotSize();
    float radius = std::min(plotSize.x, plotSize.y) * 0.4f;

    std::vector<float> maxVals(axisCount, 0.0f);
    for (int a = 0; a < axisCount; ++a)
        for (float v : g_ActiveFeatures[a].series.values) maxVals[a] = std::max(maxVals[a], v);

    for (int a = 0; a < axisCount; ++a) {
        float ang = 2 * IM_PI * a / axisCount - IM_PI / 2;
        ImVec2 p = { center.x + cosf(ang) * radius, center.y + sinf(ang) * radius };
        draw->AddLine(center, p, IM_COL32(120,120,120,200), 1.0f);
        ImVec2 labelPos = { center.x + cosf(ang) * (radius + 14), center.y + sinf(ang) * (radius + 14) };
        const char* labelText = g_ActiveFeatures[a].label.c_str();
        ImVec2 textSize = ImGui::CalcTextSize(labelText);
        if (cosf(ang) < -0.1f) labelPos.x -= textSize.x;
        else if (cosf(ang) <= 0.1f) labelPos.x -= textSize.x * 0.5f;
        if (sinf(ang) < -0.1f) labelPos.y -= textSize.y * 0.5f;
        else if (sinf(ang) > 0.1f) labelPos.y += textSize.y * 0.25f;
        draw->AddText(labelPos, IM_COL32_WHITE, labelText);
        
        ImGui::SetCursorScreenPos(labelPos);
        ImGui::InvisibleButton(("##axis" + std::to_string(a)).c_str(), ImVec2(50,15));
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload("RADAR_REMOVE", &a, sizeof(int));
            ImGui::Text("Remove %s", g_ActiveFeatures[a].label.c_str());
            ImGui::EndDragDropSource();
        }
    }
    
    for (int ring = 1; ring <= 4; ++ring) {
        float rr = radius * ring / 4.0f;
        for (int a = 0; a < axisCount; ++a) {
             float a0 = 2 * IM_PI * a / axisCount - IM_PI / 2;
             float a1 = 2 * IM_PI * (a + 1) / axisCount - IM_PI / 2;
             draw->AddLine({ center.x + cosf(a0) * rr, center.y + sinf(a0) * rr },
                           { center.x + cosf(a1) * rr, center.y + sinf(a1) * rr },
                           IM_COL32(80,80,80,150), 1.0f);
        }
    }

    const size_t countryCount = g_ActiveFeatures[0].series.values.size();

    for (size_t c = 0; c < countryCount; ++c) {
        if (labels[c] == highlight) continue;

        ImVec4 colVec = ImColor::HSV((float)c / countryCount, 0.6f, 0.9f);
        ImU32 col = ImColor(colVec);
        ImU32 fillCol = IM_COL32((int)(colVec.x * 255), (int)(colVec.y * 255), (int)(colVec.z * 255), 50);
        std::vector<ImVec2> poly;

        for (int a = 0; a < axisCount; ++a) {
            float ang = 2 * IM_PI * a / axisCount - IM_PI / 2;
            float norm = maxVals[a] > 0 ? g_ActiveFeatures[a].series.values[c] / maxVals[a] : 0.0f;
            poly.push_back({ center.x + cosf(ang) * (norm * radius), center.y + sinf(ang) * (norm * radius) });
        }

        if (poly.size() >= 3) {
            for (int i = 0; i < axisCount; ++i) draw->AddTriangleFilled(center, poly[i], poly[(i+1)%axisCount], fillCol);
        }
        for (int i = 0; i < axisCount; ++i) {
            draw->AddLine(poly[i], poly[(i+1)%axisCount], col, 2.0f);
        }
    }

    if (!highlight.empty()) {
        for (size_t c = 0; c < countryCount; ++c) {
            if (labels[c] != highlight) continue;

            ImU32 col = IM_COL32(255, 215, 0, 255); // Gold
            ImU32 fillCol = IM_COL32(255, 215, 0, 100);
            std::vector<ImVec2> poly;

            for (int a = 0; a < axisCount; ++a) {
                float ang = 2 * IM_PI * a / axisCount - IM_PI / 2;
                float norm = maxVals[a] > 0 ? g_ActiveFeatures[a].series.values[c] / maxVals[a] : 0.0f;
                poly.push_back({ center.x + cosf(ang) * (norm * radius), center.y + sinf(ang) * (norm * radius) });
            }

            if (poly.size() >= 3) {
                for (int i = 0; i < axisCount; ++i) draw->AddTriangleFilled(center, poly[i], poly[(i+1)%axisCount], fillCol);
            }
            for (int i = 0; i < axisCount; ++i) {
                draw->AddLine(poly[i], poly[(i+1)%axisCount], col, 4.0f);
                draw->AddCircleFilled(poly[i], 5.0f, col);
            }
        }
    }

    ImPlot::EndPlot();
    ImGui::EndGroup();
    ImGui::EndGroup();
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
        float diagonal = textSize.x * cosf(angle);
        pPix.x += textSize.x * sinf(angle);
        pPix.y += yOffset + diagonal;
        AddTextRotated(drawList, font, fontSize, pPix.x, pPix.y, IM_COL32(255,255,255,255), labels[i].c_str(), angle);
    }
}
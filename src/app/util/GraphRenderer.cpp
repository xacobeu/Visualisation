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

std::vector<GraphRenderer::SplomFeature> GraphRenderer::g_SplomAvailableFeatures;
std::vector<GraphRenderer::SplomFeature> GraphRenderer::g_SplomActiveFeatures;

bool GraphRenderer::s_IsDragging = false;
bool GraphRenderer::s_HasSelection = false;
bool GraphRenderer::s_HasFilter = false;
float GraphRenderer::s_DragStartX = 0.0f;
float GraphRenderer::s_DragStartY = 0.0f;
float GraphRenderer::s_DragEndX = 0.0f;
float GraphRenderer::s_DragEndY = 0.0f;
int GraphRenderer::s_DragPlotRow = -1;
int GraphRenderer::s_DragPlotCol = -1;
std::vector<bool> GraphRenderer::s_HighlightedPoints;
std::vector<bool> GraphRenderer::s_FilteredPoints;

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

void GraphRenderer::Render(const GraphSpec& spec,
                           const std::vector<std::string>& itemLabels,
                           std::vector<PlotSeries>& data,
                           const std::string& highlight,
                           const std::unordered_map<std::string, std::string>& continentMap)
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
            RenderTreeMap(spec, itemLabels, data, highlight, continentMap);
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
                                  const std::string& highlight,
                                  const std::unordered_map<std::string, std::string>& continentMap)
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

    // 2. Prepare Data - Group by continent if continent map is provided
    std::unordered_map<std::string, std::vector<TreemapNode>> continentGroups;
    std::unordered_map<std::string, float> continentTotals;
    float totalValue = 0.0f;
    
    for (size_t i = 0; i < labels.size(); ++i) {
        float val = data[0].values[i];
        if (val > 0) {
            TreemapNode node;
            node.label = labels[i];
            node.value = val;
            node.originalIndex = (int)i;
            
            // Get continent for this country
            auto it = continentMap.find(labels[i]);
            node.continent = (it != continentMap.end()) ? it->second : "Unknown";
            
            continentGroups[node.continent].push_back(node);
            continentTotals[node.continent] += val;
            totalValue += val;
        }
    }

    if (totalValue <= 0) {
        ImGui::Dummy(avail);
        return;
    }

    // If no continents provided or only one continent, render flat
    if (continentMap.empty() || continentGroups.size() <= 1) {
        // Flat rendering (original behavior)
        std::vector<TreemapNode> nodes;
        for (auto& [continent, countries] : continentGroups) {
            nodes.insert(nodes.end(), countries.begin(), countries.end());
        }
        
        std::sort(nodes.begin(), nodes.end(), [](const TreemapNode& a, const TreemapNode& b) {
            return a.value > b.value;
        });

        float totalArea = avail.x * avail.y;
        for (auto& node : nodes) {
            node.area = (node.value / totalValue) * totalArea;
        }

        Rect remainingRect = { p0.x, p0.y, avail.x, avail.y };
        std::vector<TreemapNode*> currentRow;
        
        for (size_t i = 0; i < nodes.size(); ++i) {
            TreemapNode* node = &nodes[i];
            
            if (currentRow.empty()) {
                currentRow.push_back(node);
            } else {
                float side = remainingRect.shortestSide();
                float currentWorst = WorstAspectRatio(currentRow, side);
                std::vector<TreemapNode*> proposedRow = currentRow;
                proposedRow.push_back(node);
                float proposedWorst = WorstAspectRatio(proposedRow, side);
                
                if (currentWorst >= proposedWorst) {
                    currentRow.push_back(node);
                } else {
                    LayoutRow(currentRow, remainingRect, remainingRect.w < remainingRect.h);
                    currentRow.clear();
                    currentRow.push_back(node);
                }
            }
        }

        if (!currentRow.empty()) {
            LayoutRow(currentRow, remainingRect, remainingRect.w < remainingRect.h);
        }

        for (size_t i = 0; i < nodes.size(); ++i) {
            float percent = (totalValue > 0) ? (nodes[i].value / totalValue) * 100.0f : 0.0f;
            DrawTreemapNode(drawList, nodes[i], highlight, spec, data[0], percent);
        }
    } else {
        // Hierarchical rendering by continent
        // Create continent-level nodes
        std::vector<std::pair<std::string, float>> continentList;
        for (const auto& [continent, total] : continentTotals) {
            continentList.push_back({continent, total});
        }
        
        // Sort continents by total value
        std::sort(continentList.begin(), continentList.end(), 
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Calculate continent areas
        float totalArea = avail.x * avail.y;
        std::vector<Rect> continentRects;
        std::vector<TreemapNode> continentNodes;
        
        for (const auto& [continent, total] : continentList) {
            TreemapNode contNode;
            contNode.label = continent;
            contNode.value = total;
            contNode.area = (total / totalValue) * totalArea;
            continentNodes.push_back(contNode);
        }
        
        // Layout continents
        Rect remainingRect = { p0.x, p0.y, avail.x, avail.y };
        std::vector<TreemapNode*> currentRow;
        
        for (size_t i = 0; i < continentNodes.size(); ++i) {
            TreemapNode* node = &continentNodes[i];
            
            if (currentRow.empty()) {
                currentRow.push_back(node);
            } else {
                float side = remainingRect.shortestSide();
                float currentWorst = WorstAspectRatio(currentRow, side);
                std::vector<TreemapNode*> proposedRow = currentRow;
                proposedRow.push_back(node);
                float proposedWorst = WorstAspectRatio(proposedRow, side);
                
                if (currentWorst >= proposedWorst) {
                    currentRow.push_back(node);
                } else {
                    LayoutRow(currentRow, remainingRect, remainingRect.w < remainingRect.h);
                    currentRow.clear();
                    currentRow.push_back(node);
                }
            }
        }
        
        if (!currentRow.empty()) {
            LayoutRow(currentRow, remainingRect, remainingRect.w < remainingRect.h);
        }
        
        // Now render each continent with its countries
        for (size_t i = 0; i < continentNodes.size(); ++i) {
            const auto& contNode = continentNodes[i];
            const std::string& continent = contNode.label;
            auto& countries = continentGroups[continent];
            
            // Sort countries within continent
            std::sort(countries.begin(), countries.end(), [](const TreemapNode& a, const TreemapNode& b) {
                return a.value > b.value;
            });
            
            // Calculate country areas within continent rect
            float continentTotal = continentTotals[continent];
            float continentArea = contNode.w * contNode.h;
            
            for (auto& country : countries) {
                country.area = (country.value / continentTotal) * continentArea;
            }
            
            // Layout countries within continent
            Rect countryRect = { contNode.x, contNode.y, contNode.w, contNode.h };
            std::vector<TreemapNode*> countryRow;
            
            for (size_t j = 0; j < countries.size(); ++j) {
                TreemapNode* cnode = &countries[j];
                
                if (countryRow.empty()) {
                    countryRow.push_back(cnode);
                } else {
                    float side = countryRect.shortestSide();
                    float currentWorst = WorstAspectRatio(countryRow, side);
                    std::vector<TreemapNode*> proposedRow = countryRow;
                    proposedRow.push_back(cnode);
                    float proposedWorst = WorstAspectRatio(proposedRow, side);
                    
                    if (currentWorst >= proposedWorst) {
                        countryRow.push_back(cnode);
                    } else {
                        LayoutRow(countryRow, countryRect, countryRect.w < countryRect.h);
                        countryRow.clear();
                        countryRow.push_back(cnode);
                    }
                }
            }
            
            if (!countryRow.empty()) {
                LayoutRow(countryRow, countryRect, countryRect.w < countryRect.h);
            }
            
            // Draw continent border
            ImVec2 cMin(contNode.x, contNode.y);
            ImVec2 cMax(contNode.x + contNode.w, contNode.y + contNode.h);
            drawList->AddRect(cMin, cMax, IM_COL32(255, 255, 255, 150), 0.0f, 0, 3.0f);
            
            // Draw continent label at top
            ImVec2 textPos(contNode.x + 5, contNode.y + 3);
            drawList->AddText(textPos, IM_COL32(255, 255, 255, 255), continent.c_str());
            
            // Draw countries
            for (const auto& country : countries) {
                float percent = (totalValue > 0) ? (country.value / totalValue) * 100.0f : 0.0f;
                DrawTreemapNode(drawList, country, highlight, spec, data[0], percent);
            }
        }
    }
    
    ImGui::Dummy(avail);
    
    // Add legend for continent colors
    if (!continentMap.empty() && continentGroups.size() > 1) {

        auto getContinentColor = [](const std::string& continent) -> ImU32 {
            // Perceptually balanced palette (Luminance ~60)
            if (continent == "Africa")        return IM_COL32(232, 122, 144, 255); // Rose
            if (continent == "Asia")          return IM_COL32(193, 146, 62,  255); // Ochre
            if (continent == "Europe")        return IM_COL32(102, 166, 115, 255); // Sage
            if (continent == "North America") return IM_COL32(0,   169, 181, 255); // Teal
            if (continent == "South America") return IM_COL32(121, 147, 229, 255); // Azure
            if (continent == "Oceania")       return IM_COL32(209, 123, 217, 255); // Violet
            
            // Default: Neutral Grey (Balanced Luminance)
            return IM_COL32(160, 160, 160, 255); 
        };
        
        ImGui::Spacing();
        ImGui::Text("Legend:");
        ImGui::SameLine();
        
        // Sort continents by total value for consistent legend order
        std::vector<std::pair<std::string, float>> sortedContinents;
        for (const auto& [continent, total] : continentTotals) {
            sortedContinents.push_back({continent, total});
        }
        std::sort(sortedContinents.begin(), sortedContinents.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        for (size_t i = 0; i < sortedContinents.size(); ++i) {
            const auto& [continent, total] = sortedContinents[i];
            ImU32 color = getContinentColor(continent);
            
            if (i > 0) ImGui::SameLine();
            
            // Draw color box
            ImVec2 cursorPos = ImGui::GetCursorScreenPos();
            drawList->AddRectFilled(cursorPos, ImVec2(cursorPos.x + 15, cursorPos.y + 15), color);
            drawList->AddRect(cursorPos, ImVec2(cursorPos.x + 15, cursorPos.y + 15), IM_COL32(255, 255, 255, 100));
            ImGui::Dummy(ImVec2(15, 15));
            
            ImGui::SameLine();
            ImGui::Text("%s", continent.c_str());
        }
    }
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
                                    const std::string& highlight, const GraphSpec& spec, const PlotSeries& dataSeries, float percent)
{
    ImDrawList* drawList = (ImDrawList*)drawListPtr;
    ImVec2 pMin(node.x, node.y);
    ImVec2 pMax(node.x + node.w, node.y + node.h);
    // Padding
    pMin.x += 1; pMin.y += 1;
    pMax.x -= 1; pMax.y -= 1;
    if (pMax.x <= pMin.x || pMax.y <= pMin.y) return;
    
    auto getContinentColor = [](const std::string& continent) -> ImU32 {
        // Perceptually balanced palette (Luminance ~60)
        if (continent == "Africa")        return IM_COL32(232, 122, 144, 255); // Rose
        if (continent == "Asia")          return IM_COL32(193, 146, 62,  255); // Ochre
        if (continent == "Europe")        return IM_COL32(102, 166, 115, 255); // Sage
        if (continent == "North America") return IM_COL32(0,   169, 181, 255); // Teal
        if (continent == "South America") return IM_COL32(121, 147, 229, 255); // Azure
        if (continent == "Oceania")       return IM_COL32(209, 123, 217, 255); // Violet
        
        // Default: Neutral Grey (Balanced Luminance)
        return IM_COL32(160, 160, 160, 255); 
    };
    
    // Color logic
    ImU32 col;
    bool isHighlighted = (!highlight.empty() && node.label == highlight);
    bool isBrushed = s_HasSelection && node.originalIndex >= 0 && 
                     node.originalIndex < (int)s_HighlightedPoints.size() && 
                     s_HighlightedPoints[node.originalIndex];
    bool isFiltered = s_HasFilter && node.originalIndex >= 0 &&
                      node.originalIndex < (int)s_FilteredPoints.size() &&
                      s_FilteredPoints[node.originalIndex];
    
    if (isHighlighted) {
        col = IM_COL32(255, 200, 50, 255); // Bright Yellow/Orange for hover
    } else if (isBrushed || isFiltered) {
        col = IM_COL32(255, 153, 26, 255); // Bright orange for brushed
    } else if (s_HasSelection || s_HasFilter) {
        // Dimmed when there's a selection but this node isn't selected
        ImU32 baseCol = getContinentColor(node.continent);
        ImVec4 colVec = ImGui::ColorConvertU32ToFloat4(baseCol);
        col = ImColor(colVec.x, colVec.y, colVec.z, 0.3f);
    } else {
        // Use continent color
        col = getContinentColor(node.continent);
    }
    drawList->AddRectFilled(pMin, pMax, col);
    
    if (isHighlighted) {
         drawList->AddRect(pMin, pMax, IM_COL32(255, 255, 255, 255), 0.0f, 0, 3.0f);
    } else if (isBrushed || isFiltered) {
         drawList->AddRect(pMin, pMax, IM_COL32(255, 200, 100, 255), 0.0f, 0, 2.0f);
    }
    // Hover
    if (ImGui::IsMouseHoveringRect(pMin, pMax)) {
         drawList->AddRect(pMin, pMax, IM_COL32(255, 255, 255, 150), 0.0f, 0, 2.0f);
         ImGui::BeginTooltip();
         ImGui::Text("%s (%.1f%%)", node.label.c_str(), percent);
         ImGui::Text("%s: %.2f %s", spec.series.labels[0].c_str(), node.value, dataSeries.unit.c_str());
         ImGui::EndTooltip();
    }
    // Label if fits
    if (node.w > 30 && node.h > 15) {
        char labelBuf[128];
        snprintf(labelBuf, sizeof(labelBuf), "%s (%.1f%%)", node.label.c_str(), percent);
        std::string label = labelBuf;
        ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
        // Truncate if too long
        if (textSize.x > node.w - 4) {
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
    const bool hasFilter = s_HasFilter && s_FilteredPoints.size() == labels.size();

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

        if (hasFilter) {
            for (size_t k = 0; k < labels.size(); ++k) {
                if (s_FilteredPoints[k]) {
                    double h_x = xOffset[k];
                    double h_y = s.values[k];
                    ImPlot::PushStyleColor(ImPlotCol_Fill, ImVec4(0.3f, 0.9f, 0.4f, 0.9f));
                    ImPlot::PlotBars("##FilteredHighlight", &h_x, &h_y, 1, barWidth);
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
        bool isFiltered = s_HasFilter && i < s_FilteredPoints.size() && s_FilteredPoints[i];
        if (isHigh) {
            ImPlot::PushStyleColor(ImPlotCol_MarkerFill, ImVec4(1, 0.8f, 0, 1)); 
            ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, ImVec4(1, 0.8f, 0, 1)); 
            ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, 8.0f);
        } else if (isFiltered) {
            ImPlot::PushStyleColor(ImPlotCol_MarkerFill, ImVec4(0.3f, 0.9f, 0.4f, 1.0f));
            ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, ImVec4(0.5f, 1.0f, 0.6f, 1.0f));
            ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, 7.0f);
        }

        ImPlot::PlotScatter(labels[i].c_str(), &data[0].values[i], &data[1].values[i], 1);
        
        if (isHigh || isFiltered) {
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
    const int totalFeatures = static_cast<int>(data.size());
    if (totalFeatures < 2) {
        ImGui::Text("SPLOM requires at least 2 data series.");
        return;
    }

    ImGui::TextUnformatted(spec.title.c_str());

    // Initialize features if needed
    if ((g_SplomActiveFeatures.empty() && g_SplomAvailableFeatures.empty()) || 
        (g_SplomActiveFeatures.size() + g_SplomAvailableFeatures.size()) != data.size()) {
        g_SplomActiveFeatures.clear();
        g_SplomAvailableFeatures.clear();

        for (size_t i = 0; i < data.size(); ++i) {
            SplomFeature feature;
            feature.id = (int)i;
            feature.label = (i < spec.series.labels.size()) ? spec.series.labels[i] : data[i].name;
            feature.series = data[i];
            if (i < 2) {
                g_SplomActiveFeatures.push_back(feature);
            } else {
                g_SplomAvailableFeatures.push_back(feature);
            }
        }
    } else {
        // Update existing features with new data
        for (auto& feature : g_SplomActiveFeatures) 
            if (feature.id >= 0 && feature.id < (int)data.size()) 
                feature.series = data[feature.id];
        for (auto& feature : g_SplomAvailableFeatures) 
            if (feature.id >= 0 && feature.id < (int)data.size()) 
                feature.series = data[feature.id];
    }

    // Start layout
    ImGui::BeginGroup();
    
    // Build active data and labels from g_SplomActiveFeatures
    std::vector<PlotSeries> activeData;
    std::vector<std::string> activeLabels;
    for (const auto& feature : g_SplomActiveFeatures) {
        activeData.push_back(feature.series);
        activeLabels.push_back(feature.label);
    }
    
    const int n = static_cast<int>(activeData.size());
    
    // Main SPLOM display area
    if (n < 1) {
        ImGui::BeginChild("SplomPlaceholder", ImVec2(-1, 400), true, ImGuiWindowFlags_NoScrollbar);
        ImVec2 avail = ImGui::GetContentRegionAvail();
        ImVec2 textSize = ImGui::CalcTextSize("Drop features here to display SPLOM");
        ImGui::SetCursorPos(ImVec2((avail.x - textSize.x) * 0.5f, (avail.y - textSize.y) * 0.5f));
        ImGui::TextDisabled("Drop features here to display SPLOM");
        ImGui::SetCursorPos(ImVec2(0, 0));
        ImGui::InvisibleButton("##dropzone", avail);
        
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPLOM_FEATURE")) {
                int src = *(int*)payload->Data;
                g_SplomActiveFeatures.push_back(g_SplomAvailableFeatures[src]);
                g_SplomAvailableFeatures.erase(g_SplomAvailableFeatures.begin() + src);
                s_HasSelection = false;
                s_IsDragging = false;
                s_HighlightedPoints.clear();
            }
            ImGui::EndDragDropTarget();
        }
        
        ImGui::EndChild();
    } else {
        // Render SPLOM with active features
        std::vector<float> seriesMin(n), seriesMax(n);
    for (int i = 0; i < n; ++i) {
        if (activeData[i].values.empty()) continue;
        auto [minIt, maxIt] = std::minmax_element(activeData[i].values.begin(), activeData[i].values.end());
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
                    const std::string& label = activeLabels[col];
                    float wrapWidth = cellSize - 6.0f;
                    
                    ImGui::SetWindowFontScale(0.85f);
                    ImVec2 textSize = ImGui::CalcTextSize(label.c_str(), nullptr, false, wrapWidth);
                    float offsetX = (cellSize - textSize.x) * 0.5f;
                    if (offsetX > 0) {
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offsetX);
                    }
                    
                    ImVec2 textStartPos = ImGui::GetCursorScreenPos();
                    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + wrapWidth);
                    ImGui::TextWrapped("%s", label.c_str());
                    ImGui::SetWindowFontScale(1.0f);
                    ImGui::PopTextWrapPos();
                    
                    // Make label draggable
                    ImGui::SetCursorScreenPos(textStartPos);
                    ImGui::InvisibleButton(("##xlabel" + std::to_string(col)).c_str(), textSize);
                    if (ImGui::BeginDragDropSource()) {
                        ImGui::SetDragDropPayload("SPLOM_REMOVE", &col, sizeof(int));
                        ImGui::Text("Remove %s", label.c_str());
                        ImGui::EndDragDropSource();
                    }
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
                const std::string& label = activeLabels[row];
                ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, label.c_str());
                float textOffset = textSize.x * 0.5f;
                AddTextRotated(drawList, font, fontSize, cellCenterX - 6, cellCenterY + textOffset, IM_COL32(255, 255, 255, 255), label.c_str(), -IM_PI / 2.0f);
                
                // Make label draggable
                ImGui::SetCursorScreenPos(ImVec2(cellPos.x, cellPos.y + (cellSize - textSize.x) * 0.5f));
                ImGui::InvisibleButton(("##ylabel" + std::to_string(row)).c_str(), ImVec2(labelColWidth, textSize.x));
                if (ImGui::BeginDragDropSource()) {
                    ImGui::SetDragDropPayload("SPLOM_REMOVE", &row, sizeof(int));
                    ImGui::Text("Remove %s", label.c_str());
                    ImGui::EndDragDropSource();
                }
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
                        // Draw histogram on diagonal
                        const int numBins = 10;
                        std::vector<double> bins(numBins + 1);
                        std::vector<double> counts(numBins, 0.0);
                        
                        // Create bin edges
                        float range = seriesMax[col] - seriesMin[col];
                        for (int b = 0; b <= numBins; ++b) {
                            bins[b] = seriesMin[col] + (range * b / numBins);
                        }
                        
                        // Count values in each bin
                        for (const auto& val : activeData[col].values) {
                            if (val >= seriesMin[col] && val <= seriesMax[col]) {
                                int binIdx = static_cast<int>((val - seriesMin[col]) / range * numBins);
                                if (binIdx >= numBins) binIdx = numBins - 1;
                                if (binIdx < 0) binIdx = 0;
                                counts[binIdx] += 1.0;
                            }
                        }
                        
                        // Find max count for Y-axis scaling
                        double maxCount = *std::max_element(counts.begin(), counts.end());
                        if (maxCount > 0) {
                            ImPlot::SetupAxisLimits(ImAxis_Y1, 0, maxCount * 1.1, ImPlotCond_Always);
                        }
                        
                        // Draw histogram bars
                        ImPlot::PushStyleColor(ImPlotCol_Fill, ImVec4(0.2f, 0.5f, 0.2f, 0.8f));
                        for (int b = 0; b < numBins; ++b) {
                            double barWidth = bins[b + 1] - bins[b];
                            double barCenter = (bins[b] + bins[b + 1]) / 2.0;
                            ImPlot::PlotBars("##histogram", &barCenter, &counts[b], 1, barWidth);
                        }
                        ImPlot::PopStyleColor();
                        
                        // Add drop target to plot area
                        if (ImPlot::BeginDragDropTargetPlot()) {
                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPLOM_FEATURE")) {
                                int src = *(int*)payload->Data;
                                g_SplomActiveFeatures.push_back(g_SplomAvailableFeatures[src]);
                                g_SplomAvailableFeatures.erase(g_SplomAvailableFeatures.begin() + src);
                                // Clear selection state when features change
                                s_HasSelection = false;
                                s_IsDragging = false;
                                s_HighlightedPoints.clear();
                            }
                            ImPlot::EndDragDropTarget();
                        }
                        
                        ImPlot::EndPlot();
                        ImPlot::PopStyleColor();
                        ImPlot::PopStyleVar();
                        continue;
                    }

                    if (s_HighlightedPoints.size() != labels.size()) s_HighlightedPoints.resize(labels.size(), false);

                    // First pass: draw all non-hover-highlighted points
                    for (size_t i = 0; i < labels.size(); ++i) {
                        bool isHoverHighlight = (!highlight.empty() && labels[i] == highlight);
                        if (isHoverHighlight) continue; // Skip hover highlight in first pass
                        
                        bool isDragSelected = s_HasSelection && s_HighlightedPoints[i];
                        bool isFilterSelected = s_HasFilter && i < s_FilteredPoints.size() && s_FilteredPoints[i];

                        if (isDragSelected) {
                            ImPlot::PushStyleColor(ImPlotCol_MarkerFill, ImVec4(1.0f, 0.6f, 0.1f, 1.0f));
                            ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, ImVec4(1.0f, 0.8f, 0.3f, 1.0f));
                            ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, 7.0f);
                        } else if (isFilterSelected) {
                            ImPlot::PushStyleColor(ImPlotCol_MarkerFill, ImVec4(0.3f, 0.9f, 0.4f, 1.0f));
                            ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, ImVec4(0.5f, 1.0f, 0.6f, 1.0f));
                            ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, 7.0f);
                        } else if (s_HasSelection || s_HasFilter) {
                            ImPlot::PushStyleColor(ImPlotCol_MarkerFill, ImVec4(0.2f, 0.5f, 0.2f, 0.3f));
                            ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, ImVec4(0.15f, 0.4f, 0.15f, 0.3f));
                        } else {
                            ImPlot::PushStyleColor(ImPlotCol_MarkerFill, ImVec4(0.2f, 0.5f, 0.2f, 0.8f));
                            ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, ImVec4(0.15f, 0.4f, 0.15f, 1.0f));
                        }
                        
                        ImPlot::PlotScatter(("##" + labels[i]).c_str(), &activeData[col].values[i], &activeData[row].values[i], 1);
                        
                        ImPlot::PopStyleColor(2);
                        if (isDragSelected || isFilterSelected) ImPlot::PopStyleVar();
                    }
                    
                    // Second pass: draw hover-highlighted point on top
                    if (!highlight.empty()) {
                        for (size_t i = 0; i < labels.size(); ++i) {
                            bool isHoverHighlight = (labels[i] == highlight);
                            if (!isHoverHighlight) continue;
                            
                            ImPlot::PushStyleColor(ImPlotCol_MarkerFill, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
                            ImPlot::PushStyleColor(ImPlotCol_MarkerOutline, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                            ImPlot::PushStyleVar(ImPlotStyleVar_MarkerSize, 8.0f);
                            
                            ImPlot::PlotScatter(("##" + labels[i]).c_str(), &activeData[col].values[i], &activeData[row].values[i], 1);
                            
                            ImPlot::PopStyleColor(2);
                            ImPlot::PopStyleVar();
                            break; // Only one hover highlight
                        }
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
                                bool anySelected = false;
                                for (size_t i = 0; i < labels.size(); ++i) {
                                    float px = activeData[col].values[i];
                                    float py = activeData[row].values[i];
                                    s_HighlightedPoints[i] = (px >= minX && px <= maxX && py >= minY && py <= maxY);
                                    if (s_HighlightedPoints[i]) anySelected = true;
                                }
                                // If no points were selected, clear the selection
                                if (!anySelected) {
                                    s_HasSelection = false;
                                    std::fill(s_HighlightedPoints.begin(), s_HighlightedPoints.end(), false);
                                }
                            }
                        }
                    }

                    
                    if (ImPlot::IsPlotHovered()) {

                        float minDist = FLT_MAX;
                        int closestIdx = -1;

                        for (size_t i = 0; i < labels.size(); ++i) {
                            ImVec2 dataPix = ImPlot::PlotToPixels(activeData[col].values[i], activeData[row].values[i]);
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
                            ImGui::Text("X: %.2f", activeData[col].values[closestIdx]);
                            ImGui::Text("Y: %.2f", activeData[row].values[closestIdx]);
                            ImGui::EndTooltip();
                        }
                    }
                    
                    // Add drop target to plot area
                    if (ImPlot::BeginDragDropTargetPlot()) {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPLOM_FEATURE")) {
                            int src = *(int*)payload->Data;
                            g_SplomActiveFeatures.push_back(g_SplomAvailableFeatures[src]);
                            g_SplomAvailableFeatures.erase(g_SplomAvailableFeatures.begin() + src);
                            // Clear selection state when features change
                            s_HasSelection = false;
                            s_IsDragging = false;
                            s_HighlightedPoints.clear();
                        }
                        ImPlot::EndDragDropTarget();
                    }
                    
                    ImPlot::EndPlot();
                    ImPlot::PopStyleColor();
                    ImPlot::PopStyleVar();
                }
            }
        }
        ImGui::EndTable();
    }
    } // End of else block for n >= 1
    
    // Feature panel at the bottom - wrap layout (always shown)
    ImGui::BeginChild("SplomFeaturePanel", ImVec2(0, 100), true, 0);
    
    // Show placeholder text when empty, otherwise show features
    if (g_SplomAvailableFeatures.empty()) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        const char* placeholderText = "Drag and drop features here to remove from SPLOM";
        ImVec2 textSize = ImGui::CalcTextSize(placeholderText);
        ImGui::SetCursorPos(ImVec2((avail.x - textSize.x) * 0.5f, (avail.y - textSize.y) * 0.5f));
        ImGui::TextDisabled("%s", placeholderText);
    } else {

        // Available features list 
        for (size_t i = 0; i < g_SplomAvailableFeatures.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            ImGui::Button(g_SplomAvailableFeatures[i].label.c_str());
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                int idx = static_cast<int>(i);
                ImGui::SetDragDropPayload("SPLOM_FEATURE", &idx, sizeof(int));
                ImGui::Text("Add: %s", g_SplomAvailableFeatures[i].label.c_str());
                ImGui::EndDragDropSource();
            }
            ImGui::PopID();
        }
    }
    
    // Drop target inside the list area
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPLOM_REMOVE")) {
            int idx = *(int*)payload->Data;
            if (idx >= 0 && idx < (int)g_SplomActiveFeatures.size()) {
                g_SplomAvailableFeatures.push_back(g_SplomActiveFeatures[idx]);
                g_SplomActiveFeatures.erase(g_SplomActiveFeatures.begin() + idx);
                // Clear selection state when features change
                s_HasSelection = false;
                s_IsDragging = false;
                s_HighlightedPoints.clear();
            }
        }
        ImGui::EndDragDropTarget();
    }
    ImGui::EndChild();
    
    // Also make the child window itself a drop target
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SPLOM_REMOVE")) {
            int idx = *(int*)payload->Data;
            if (idx >= 0 && idx < (int)g_SplomActiveFeatures.size()) {
                g_SplomAvailableFeatures.push_back(g_SplomActiveFeatures[idx]);
                g_SplomActiveFeatures.erase(g_SplomActiveFeatures.begin() + idx);
                // Clear selection state when features change
                s_HasSelection = false;
                s_IsDragging = false;
                s_HighlightedPoints.clear();
            }
        }
        ImGui::EndDragDropTarget();
    }
    
    ImGui::EndGroup();  // Close the main SPLOM group
    
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
    // 1. Prepare Features
    if ((g_ActiveFeatures.empty() && g_AvailableFeatures.empty()) || 
        (g_ActiveFeatures.size() + g_AvailableFeatures.size()) != data.size()) {
        g_ActiveFeatures.clear();
        g_AvailableFeatures.clear();
        
        for (size_t i = 0; i < data.size(); ++i) {
            RadarFeature feature;
            feature.id = (int)i;
            feature.label = (i < spec.series.labels.size()) ? spec.series.labels[i] : data[i].name;
            feature.series = data[i];
            if (i < 3) {
                g_ActiveFeatures.push_back(feature);
            } else {
                g_AvailableFeatures.push_back(feature);
            }
        }
    } else {
        for (auto& feature : g_ActiveFeatures) if (feature.id >= 0 && feature.id < (int)data.size()) feature.series = data[feature.id];
        for (auto& feature : g_AvailableFeatures) if (feature.id >= 0 && feature.id < (int)data.size()) feature.series = data[feature.id];
    }

    ImGui::BeginGroup();

    // 2. Layout Calculation
    ImVec2 availRegion = ImGui::GetContentRegionAvail();
    float footerHeight = 60.0f + ImGui::GetStyle().ItemSpacing.y;
    // Ensure plot has minimum height to avoid collapse, but fill available space otherwise
    float plotHeight = std::max(availRegion.y - footerHeight, 300.0f);

    if (!ImPlot::BeginPlot(spec.title.c_str(), ImVec2(-1, plotHeight), ImPlotFlags_NoLegend | ImPlotFlags_NoMenus | ImPlotFlags_NoFrame | ImPlotFlags_NoMouseText)) {
        ImGui::EndGroup(); return;
    }
    ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoHighlight, ImPlotAxisFlags_NoDecorations | ImPlotAxisFlags_NoHighlight);
    ImPlot::SetupAxisLimits(ImAxis_X1, -1, 1, ImPlotCond_Always);
    ImPlot::SetupAxisLimits(ImAxis_Y1, -1, 1, ImPlotCond_Always);
    
    // Global drop target for the plot
    if (ImPlot::BeginDragDropTargetPlot()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RADAR_FEATURE")) {
            int src = *(int*)payload->Data;
            g_ActiveFeatures.push_back(g_AvailableFeatures[src]);
            g_AvailableFeatures.erase(g_AvailableFeatures.begin() + src);
        }
        ImPlot::EndDragDropTarget();
    }

    const int axisCount = (int)g_ActiveFeatures.size();
    if (axisCount == 0) { 
        ImPlot::EndPlot(); 
        goto RenderFooter; 
    }

    // --- DRAWING LOGIC ---
    {
        ImDrawList* draw = ImPlot::GetPlotDrawList();
        ImVec2 center = ImPlot::PlotToPixels({0, 0});
        ImVec2 plotSize = ImPlot::GetPlotSize();
        float radius = std::min(plotSize.x, plotSize.y) * 0.4f;

        std::vector<float> maxVals(axisCount, 0.0f);
        for (int a = 0; a < axisCount; ++a)
            for (float v : g_ActiveFeatures[a].series.values) maxVals[a] = std::max(maxVals[a], v);

        // Draw Axes and Labels
        for (int a = 0; a < axisCount; ++a) {
            float ang = 2 * IM_PI * a / axisCount - IM_PI / 2;
            ImVec2 p = { center.x + cosf(ang) * radius, center.y + sinf(ang) * radius };
            draw->AddLine(center, p, IM_COL32(120,120,120,200), 1.0f);
            
            ImVec2 labelPos = { center.x + cosf(ang) * (radius + 14), center.y + sinf(ang) * (radius + 14) };
            const char* labelText = g_ActiveFeatures[a].label.c_str();
            ImVec2 textSize = ImGui::CalcTextSize(labelText);
            
            // Adjust label alignment
            if (cosf(ang) < -0.1f) labelPos.x -= textSize.x;
            else if (cosf(ang) <= 0.1f) labelPos.x -= textSize.x * 0.5f;
            if (sinf(ang) < -0.1f) labelPos.y -= textSize.y * 0.5f;
            else if (sinf(ang) > 0.1f) labelPos.y += textSize.y * 0.25f;
            
            draw->AddText(labelPos, IM_COL32_WHITE, labelText);
            
            // [CRITICAL FIX START] ---------------------------------------------
            // We must save the cursor position before moving it to place the button.
            // If we don't, the next UI element (the Footer) will draw starting from here (middle of the plot).
            ImVec2 backupCursorPos = ImGui::GetCursorScreenPos();
            
            ImGui::SetCursorScreenPos(labelPos);
            ImGui::InvisibleButton(("##axis" + std::to_string(a)).c_str(), ImVec2(textSize.x, textSize.y));
            
            if (ImGui::BeginDragDropSource()) {
                ImGui::SetDragDropPayload("RADAR_REMOVE", &a, sizeof(int));
                ImGui::Text("Remove %s", g_ActiveFeatures[a].label.c_str());
                ImGui::EndDragDropSource();
            }

            // Restore the cursor so layout flow continues correctly from the bottom of the plot later
            ImGui::SetCursorScreenPos(backupCursorPos);
            // [CRITICAL FIX END] -----------------------------------------------
        }
        
        // Grid rings
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

        // Draw Polygons
        const size_t countryCount = g_ActiveFeatures[0].series.values.size();
        std::vector<std::vector<ImVec2>> allPolygons;
        allPolygons.reserve(countryCount);

        // Ensure s_HighlightedPoints size matches - preserve existing values
        if (s_HighlightedPoints.size() != countryCount) {
            s_HighlightedPoints.resize(countryCount, false);
        }

        for (size_t c = 0; c < countryCount; ++c) {
            if (labels[c] == highlight) continue;

            bool isBrushed = s_HasSelection && c < s_HighlightedPoints.size() && s_HighlightedPoints[c];
            bool isFiltered = s_HasFilter && c < s_FilteredPoints.size() && s_FilteredPoints[c];
            
            ImVec4 colVec;
            ImU32 col;
            ImU32 fillCol;
            
            if (isBrushed || isFiltered) {
                // Brushed polygons - bright orange
                colVec = ImVec4(1.0f, 0.6f, 0.1f, 1.0f);
                col = ImColor(colVec);
                fillCol = IM_COL32(255, 153, 26, 80);
            } else if (s_HasSelection || s_HasFilter) {
                // Non-brushed polygons when there's a selection - dimmed
                colVec = ImColor::HSV((float)c / countryCount, 0.6f, 0.9f);
                col = ImColor(colVec.x, colVec.y, colVec.z, 0.2f);
                fillCol = IM_COL32((int)(colVec.x * 255), (int)(colVec.y * 255), (int)(colVec.z * 255), 10);
            } else {
                // Normal rendering - no selection active
                colVec = ImColor::HSV((float)c / countryCount, 0.6f, 0.9f);
                col = ImColor(colVec);
                fillCol = IM_COL32((int)(colVec.x * 255), (int)(colVec.y * 255), (int)(colVec.z * 255), 50);
            }

            std::vector<ImVec2> poly;

            for (int a = 0; a < axisCount; ++a) {
                float ang = 2 * IM_PI * a / axisCount - IM_PI / 2;
                float norm = maxVals[a] > 0 ? g_ActiveFeatures[a].series.values[c] / maxVals[a] : 0.0f;
                poly.push_back({ center.x + cosf(ang) * (norm * radius), center.y + sinf(ang) * (norm * radius) });
            }

            allPolygons.push_back(poly);

            if (poly.size() >= 3) {
                for (int i = 0; i < axisCount; ++i) draw->AddTriangleFilled(center, poly[i], poly[(i+1)%axisCount], fillCol);
            }
            float lineWidth = isBrushed ? 3.0f : 2.0f;
            for (int i = 0; i < axisCount; ++i) {
                draw->AddLine(poly[i], poly[(i+1)%axisCount], col, lineWidth);
            }
        }

        // Highlighted Polygon
        if (!highlight.empty()) {
            for (size_t c = 0; c < countryCount; ++c) {
                if (labels[c] != highlight) continue;

                ImU32 col = IM_COL32(255, 215, 0, 255); 
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
        
        // Tooltip logic
        if (ImPlot::IsPlotHovered()) {
            ImVec2 mousePos = ImGui::GetMousePos();
            auto isPointInPolygon = [](const ImVec2& point, const std::vector<ImVec2>& polygon) {
                bool inside = false;
                int n = (int)polygon.size();
                for (int i = 0, j = n - 1; i < n; j = i++) {
                    if (((polygon[i].y > point.y) != (polygon[j].y > point.y)) &&
                        (point.x < (polygon[j].x - polygon[i].x) * (point.y - polygon[i].y) / (polygon[j].y - polygon[i].y) + polygon[i].x)) {
                        inside = !inside;
                    }
                }
                return inside;
            };
            
            int hoveredCountry = -1;
            for (size_t c = 0; c < allPolygons.size(); ++c) {
                if (!allPolygons[c].empty() && isPointInPolygon(mousePos, allPolygons[c])) {
                    hoveredCountry = (int)c;
                    break;
                }
            }
            
            if (hoveredCountry >= 0) {
                int actualIndex = hoveredCountry;
                if (!highlight.empty()) {
                    for (int i = 0; i <= hoveredCountry; ++i) {
                        if (labels[i] == highlight) {
                            actualIndex++;
                            break;
                        }
                    }
                }
                if (actualIndex < (int)countryCount) {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", labels[actualIndex].c_str());
                    ImGui::Separator();
                    for (int a = 0; a < axisCount; ++a) {
                        ImGui::Text("%s: %.2f", g_ActiveFeatures[a].label.c_str(), g_ActiveFeatures[a].series.values[actualIndex]);
                    }
                    ImGui::EndTooltip();
                }
            }
        }
    }
    // --- END DRAWING LOGIC ---

    ImPlot::EndPlot();

RenderFooter:
    ImGui::Separator();
    
    // Feature panel at the bottom - horizontal layout
    ImGui::BeginChild("FeaturePanel", ImVec2(0, 100), true, 0);
    
    // Show placeholder text when empty, otherwise show features
    if (g_AvailableFeatures.empty()) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        const char* placeholderText = "Drag and drop features here to remove from Radar";
        ImVec2 textSize = ImGui::CalcTextSize(placeholderText);
        ImGui::SetCursorPos(ImVec2((avail.x - textSize.x) * 0.5f, (avail.y - textSize.y) * 0.5f));
        ImGui::TextDisabled("%s", placeholderText);
    } else {
        for (size_t i = 0; i < g_AvailableFeatures.size(); ++i) {
            ImGui::PushID((int)i);
            ImGui::Button(g_AvailableFeatures[i].label.c_str());
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                int idx = (int)i;
                ImGui::SetDragDropPayload("RADAR_FEATURE", &idx, sizeof(int));
                ImGui::Text("Add: %s", g_AvailableFeatures[i].label.c_str());
                ImGui::EndDragDropSource();
            }
            ImGui::PopID();
        }
    }
    
    // Drop target inside the list area
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

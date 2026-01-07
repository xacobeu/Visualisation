#include "GraphRenderer.hpp"

#include <imgui.h>
#include <implot.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <iostream>

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

    if (ImGui::BeginTable("SPLOM_TABLE", n, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_BordersInner)) {
        ImVec2 avail = ImGui::GetContentRegionAvail();
        float cellSize = avail.x / n;

        for (int row = 0; row < n; ++row) {
            ImGui::TableNextRow();
            for (int col = 0; col < n; ++col) {
                ImGui::TableSetColumnIndex(col);
                std::string id = "##sp_" + std::to_string(row) + "_" + std::to_string(col);

                if (ImPlot::BeginPlot(id.c_str(), ImVec2(cellSize, cellSize))) {
                    ImPlot::PushStyleVar(ImPlotStyleVar_FitPadding, ImVec2(0.2f, 0.2f));
                    ImPlot::PushStyleColor(ImPlotCol_FrameBg, ImVec4(0,0,0,0));

                    bool onBorderLeft = (col == 0);
                    bool onBorderBottom = (row == n-1);

                    ImPlot::SetupAxes(
                        onBorderBottom ? spec.series.labels[col].c_str() : nullptr,
                        onBorderLeft ? spec.series.labels[row].c_str() : nullptr,
                        ImPlotAxisFlags_AutoFit | (onBorderBottom ? 0 : ImPlotAxisFlags_NoTickLabels) | ImPlotAxisFlags_NoHighlight,
                        ImPlotAxisFlags_AutoFit | (onBorderLeft ? 0 : ImPlotAxisFlags_NoTickLabels) | ImPlotAxisFlags_NoHighlight
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

                    ImPlot::PushStyleColor(ImPlotCol_MarkerFill, ImVec4(0.3f, 0.7f, 1.0f, 0.8f));

                    for (size_t i = 0; i < labels.size(); ++i) {
                        ImPlot::PlotScatter(
                            ("##" + labels[i]).c_str(),
                            &data[col].values[i],
                            &data[row].values[i],
                            1
                        );
                    }

                    ImPlot::PopStyleColor();

                    if (ImPlot::IsPlotHovered()) {

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
                            ImGui::Text("X: %.2f", data[col].values[closestIdx]);
                            ImGui::Text("Y: %.2f", data[row].values[closestIdx]);
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
}

void GraphRenderer::RenderRadar(const GraphSpec& spec,
                                const std::vector<std::string>& labels,
                                const std::vector<PlotSeries>& data)
{
    if (data.empty() || labels.empty()) return;

    const int axisCount = (int)data.size();

    if (!ImPlot::BeginPlot(spec.title.c_str(), ImVec2(-1, 400),
        ImPlotFlags_NoLegend | ImPlotFlags_NoMenus | ImPlotFlags_NoFrame))
        return;

    ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
    ImPlot::SetupAxisLimits(ImAxis_X1, -1, 1, ImPlotCond_Always);
    ImPlot::SetupAxisLimits(ImAxis_Y1, -1, 1, ImPlotCond_Always);

    ImDrawList* draw = ImPlot::GetPlotDrawList();
    ImVec2 center = ImPlot::PlotToPixels({0, 0});
    ImVec2 plotSize = ImPlot::GetPlotSize();
    float radius = std::min(plotSize.x, plotSize.y) * 0.4f;

    // Normalization max values
    std::vector<float> maxVals(axisCount, 0.0f);
    for (int a = 0; a < axisCount; ++a) {
        for (float v : data[a].values) maxVals[a] = std::max(maxVals[a], v);
    }

    // Draw Grid
    for (int a = 0; a < axisCount; ++a) {
        float ang = 2 * IM_PI * a / axisCount - IM_PI / 2;
        ImVec2 p = ImVec2(center.x + cosf(ang) * radius, center.y + sinf(ang) * radius);
        draw->AddLine(center, p, IM_COL32(120,120,120,200), 1.0f);

        ImVec2 labelPos = ImVec2(center.x + cosf(ang) * (radius + 12), center.y + sinf(ang) * (radius + 12));
        draw->AddText(labelPos, IM_COL32_WHITE, spec.series.labels[a].c_str());
    }

    // Draw concentric rings
    for (int ring = 1; ring <= 4; ++ring) {
        float rr = radius * ring / 4.0f;
        for (int a = 0; a < axisCount; ++a) {
            float a0 = 2 * IM_PI * a / axisCount - IM_PI / 2;
            float a1 = 2 * IM_PI * (a+1) / axisCount - IM_PI / 2;
            draw->AddLine(
                ImVec2(center.x + cosf(a0) * rr, center.y + sinf(a0) * rr),
                ImVec2(center.x + cosf(a1) * rr, center.y + sinf(a1) * rr),
                IM_COL32(80,80,80,150), 1.0f);
        }
    }

    // Draw Polygons
    for (size_t c = 0; c < labels.size(); ++c) {
        ImU32 col = ImColor::HSV((float)c / labels.size(), 0.6f, 0.9f);
        std::vector<ImVec2> poly;

        for (int a = 0; a < axisCount; ++a) {
            float ang = 2 * IM_PI * a / axisCount - IM_PI / 2;
            float norm = maxVals[a] > 0 ? data[a].values[c] / maxVals[a] : 0.0f;
            poly.push_back(ImVec2(center.x + cosf(ang) * (norm * radius), center.y + sinf(ang) * (norm * radius)));
        }

        for (int i = 0; i < axisCount; ++i) {
            draw->AddLine(poly[i], poly[(i+1)%axisCount], col, 2.0f);
            draw->AddCircleFilled(poly[i], 3.5f, col);
        }
    }

    // Hover logic
    if (ImPlot::IsPlotHovered()) {
        ImVec2 mouse = ImGui::GetMousePos();
        float bestDist = FLT_MAX;
        int bestCountry = -1;

        for (size_t c = 0; c < labels.size(); ++c) {
            for (int a = 0; a < axisCount; ++a) {
                float ang = 2 * IM_PI * a / axisCount - IM_PI / 2;
                float norm = maxVals[a] > 0 ? data[a].values[c] / maxVals[a] : 0.0f;
                ImVec2 p = ImVec2(center.x + cosf(ang) * (norm * radius), center.y + sinf(ang) * (norm * radius));

                float d = static_cast<float>(pow(mouse.x - p.x, 2) + pow(mouse.y - p.y, 2));
                if (d < bestDist) {
                    bestDist = d;
                    bestCountry = (int)c;
                }
            }
        }

        if (bestCountry >= 0 && bestDist < 64.0f) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", labels[bestCountry].c_str());
            ImGui::Separator();
            for (int a = 0; a < axisCount; ++a) {
                ImGui::Text("%s: %.2f", spec.series.labels[a].c_str(), data[a].values[bestCountry]);
            }
            ImGui::EndTooltip();
        }
    }

    ImPlot::EndPlot();
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

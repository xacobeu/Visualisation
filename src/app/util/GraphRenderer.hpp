#pragma once

#include <string>
#include <vector>
#include "DataHandler.hpp"

enum class GraphType { Bar, Scatter, SPLOM, Radar };

struct SeriesSpec {
    std::vector<std::string> columns;
    std::vector<std::string> labels;
};

struct GraphSpec {
    GraphType type;
    std::string title;
    std::string xLabel;
    std::string yLabel;
    SeriesSpec series;
};

class GraphRenderer {
public:
    static void Render(const GraphSpec& spec,
                       const std::vector<std::string>& itemLabels,
                       std::vector<PlotSeries>& data);

private:
    static void RenderBar(const GraphSpec& spec,
                          const std::vector<std::string>& labels,
                          const std::vector<PlotSeries>& data);

    static void RenderScatter(const GraphSpec& spec,
                              const std::vector<std::string>& labels,
                              const std::vector<PlotSeries>& data);

    static void RenderSPLOM(const GraphSpec& spec,
                            const std::vector<std::string>& labels,
                            const std::vector<PlotSeries>& data);

    static void RenderRadar(const GraphSpec& spec,
                            const std::vector<std::string>& labels,
                            const std::vector<PlotSeries>& data);

    static void DrawRotatedLabels(const std::vector<std::string>& labels);

    static void AddTextRotated(void* drawListPtr, void* fontPtr, float fontSize,
                               float posX, float posY, unsigned int col,
                               const char* text, float angle);
};

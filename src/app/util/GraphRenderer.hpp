#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "DataHandler.hpp"

enum class GraphType { Bar, Scatter, SPLOM, Radar, TreeMap };

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
                       std::vector<PlotSeries>& data,
                       const std::string& highlight = "");
    
    static void Render(const GraphSpec& spec,
                       const std::vector<std::string>& itemLabels,
                       std::vector<PlotSeries>& data,
                       const std::string& highlight,
                       const std::unordered_map<std::string, std::string>& continentMap);

private:
    static void RenderBar(const GraphSpec& spec,
                          const std::vector<std::string>& labels,
                          const std::vector<PlotSeries>& data,
                          const std::string& highlight);

    static void RenderScatter(const GraphSpec& spec,
                              const std::vector<std::string>& labels,
                              const std::vector<PlotSeries>& data,
                              const std::string& highlight);

    static void RenderSPLOM(const GraphSpec& spec,
                            const std::vector<std::string>& labels,
                            const std::vector<PlotSeries>& data,
                            const std::string& highlight);

    // --- TREEMAP DEFINITIONS ---
    struct TreemapNode {
        std::string label;
        std::string continent;
        float value;          // The raw value (e.g. GDP)
        float area;           // The scaled pixel area
        float x, y, w, h;     // Final coordinates
        int originalIndex;
    };
    
    struct Rect {
        float x, y, w, h;
        float shortestSide() const { return std::min(w, h); }
    };

    static void RenderTreeMap(const GraphSpec& spec,
                              const std::vector<std::string>& labels,
                              const std::vector<PlotSeries>& data,
                              const std::string& highlight,
                              const std::unordered_map<std::string, std::string>& continentMap = {});

    // Helper to actually draw the rectangles once coordinates are calculated
    static void DrawTreemapNode(void* drawListPtr, const TreemapNode& node, 
                                const std::string& highlight, const GraphSpec& spec, const PlotSeries& dataSeries, float percent);
                                
    // Helper to check aspect ratios during layout
    static float WorstAspectRatio(const std::vector<TreemapNode*>& row, float sideLength);
    
    // Helper to finalize a row and calculate coordinates
    static void LayoutRow(std::vector<TreemapNode*>& row, Rect& container, bool vertical);

    // --- RADAR DEFINITIONS ---
    struct RadarFeature {
        int id;
        std::string label;
        PlotSeries series;
    };
    static std::vector<RadarFeature> g_AvailableFeatures;
    static std::vector<RadarFeature> g_ActiveFeatures;

    // --- SPLOM FEATURE MANAGEMENT ---
    struct SplomFeature {
        int id;
        std::string label;
        PlotSeries series;
    };
    static std::vector<SplomFeature> g_SplomAvailableFeatures;
    static std::vector<SplomFeature> g_SplomActiveFeatures;

    static void RenderRadar(const GraphSpec& spec,
                            const std::vector<std::string>& labels,
                            const std::vector<PlotSeries>& data,
                            const std::string& highlight);

    // Helpers
    static void DrawRotatedLabels(const std::vector<std::string>& labels);
    static void AddTextRotated(void* drawListPtr, void* fontPtr, float fontSize,
                               float posX, float posY, unsigned int col,
                               const char* text, float angle);

    // SPLOM state
    static bool s_IsDragging;
    static bool s_HasSelection;
    static float s_DragStartX, s_DragStartY;
    static float s_DragEndX, s_DragEndY;
    static int s_DragPlotRow, s_DragPlotCol;
    static std::vector<bool> s_HighlightedPoints;
};
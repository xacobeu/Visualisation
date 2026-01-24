#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include "util/CSVReader.hpp"

struct DataValue {
    std::variant<float, std::string> value;
    std::string unit;

    DataValue() : value(0.0f), unit("") {}
    DataValue(float v, std::string u = "") : value(v), unit(u) {}
    DataValue(std::string v, std::string u = "") : value(v), unit(u) {}

    bool isNumeric() const { return std::holds_alternative<float>(value); }
    float asFloat() const { return isNumeric() ? std::get<float>(value) : 0.0f; }
};

struct PlotSeries {
    std::string name;
    std::string unit;  // Unit for this series (extracted from column name or data)
    std::vector<float> values;
};

class DataHandler {
public:
    DataHandler();
    void init(); // Load all initial CSVs

    // Extract numerical data for a specific list of countries and columns
    std::vector<PlotSeries> collectSeries(
        const std::vector<std::string>& countries,
        const std::vector<std::string>& columns
    );
    
    // Get data for all countries for a single column (for choropleth)
    std::unordered_map<std::string, float> getColumnForAllCountries(const std::string& column);
    
    // Get data with units for all countries for a single column (for choropleth with tooltip units)
    struct ColumnData {
        std::unordered_map<std::string, float> values;
        std::string unit;  // Common unit for this column (from first non-empty entry)
    };
    ColumnData getColumnWithUnitForAllCountries(const std::string& column);
    
    // Get all available columns
    std::vector<std::string> getAvailableColumns() const;

private:
    void loadFromCSV(std::string file, std::vector<std::string> columns);
    DataValue parseValueWithUnit(const std::string& rawValue);

    CSVReader csvReader;
    std::unordered_set<std::string> loadedFiles;
    std::unordered_map<std::string, std::unordered_map<std::string, DataValue>> loadedData;

    // Constants
    static constexpr const char* COMMUNICATIONS_CSV_PATH = "res/data/cleaned_data_communications.csv";
    static constexpr const char* DEMOGRAPHICS_CSV_PATH = "res/data/cleaned_data_demographics.csv";
    static constexpr const char* ECONOMY_CSV_PATH = "res/data/cleaned_data_economy.csv";
    static constexpr const char* ENERGY_CSV_PATH = "res/data/cleaned_data_energy.csv";
    static constexpr const char* GEOGRAPHY_CSV_PATH = "res/data/cleaned_data_geography.csv";
    static constexpr const char* GOVERNMENT_CSV_PATH = "res/data/cleaned_data_government.csv";
};

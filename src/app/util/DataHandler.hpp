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

private:
    void loadFromCSV(std::string file, std::vector<std::string> columns);
    DataValue parseValueWithUnit(const std::string& rawValue);

    CSVReader csvReader;
    std::unordered_set<std::string> loadedFiles;
    std::unordered_map<std::string, std::unordered_map<std::string, DataValue>> loadedData;

    // Constants
    static constexpr const char* COMMUNICATIONS_CSV_PATH = "res/data/clean/communications.csv";
    static constexpr const char* ECONOMY_CSV_PATH = "res/data/clean/economy.csv";
    static constexpr const char* ENERGY_CSV_PATH = "res/data/clean/energy.csv";
    static constexpr const char* GEOGRAPHY_CSV_PATH = "res/data/clean/geography.csv";
    static constexpr const char* GOVERNMENT_CSV_PATH = "res/data/clean/government.csv";
};

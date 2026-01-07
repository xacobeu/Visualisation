#include "DataHandler.hpp"
#include <algorithm>
#include <cmath>

DataHandler::DataHandler() {}

void DataHandler::init() {
    loadFromCSV(COMMUNICATIONS_CSV_PATH, {
        "telephone_fixed_subscriptions_total",
        "mobile_cellular_subscriptions_total",
        "internet_users_total",
        "broadband_fixed_subscriptions_total"
    });
    loadFromCSV(ECONOMY_CSV_PATH, {
        "Real_GDP_PPP_billion_USD",
        "GDP_Official_Exchange_Rate_billion_USD",
        "Real_GDP_Growth_Rate_percent",
        "Real_GDP_per_Capita_USD",
        "Unemployment_Rate_percent",
        "Youth_Unemployment_Rate_percent",
        "Budget_billion_USD",
        "Budget_Surplus_billion_USD",
        "Budget_Deficit_percent_of_GDP",
        "Public_Debt_percent_of_GDP",
        "Exports_billion_USD",
        "Imports_billion_USD",
        "Exchange_Rate_per_USD",
        "Population_Below_Poverty_Line_percent"
    });
    loadFromCSV(ENERGY_CSV_PATH, {
        "electricity_access_percent",
        "electricity_generating_capacity_kW",
        "coal_metric_tons",
        "petroleum_bbl_per_day",
        "refined_petroleum_products_bbl_per_day",
        "refined_petroleum_exports_bbl_per_day",
        "refined_petroleum_imports_bbl_per_day",
        "natural_gas_cubic_meters",
        "carbon_dioxide_emissions_Mt"
    });
    loadFromCSV(GEOGRAPHY_CSV_PATH, {
        "Area_Total",
        "Highest_Elevation",
        "Lowest_Elevation",
        "Forest_Land",
        "Other_Land",
        "Agricultural_Land",
        "Arable_Land (%% of Total Agricultural Land)"
    });
    loadFromCSV(GOVERNMENT_CSV_PATH, {
        "Capital",
        "Government_Type",
        "Suffrage_Age"
    });
}

std::vector<PlotSeries> DataHandler::collectSeries(
    const std::vector<std::string>& countries,
    const std::vector<std::string>& columns)
{
    std::vector<PlotSeries> result;
    for (const auto& col : columns) {
        PlotSeries series;
        series.name = col;
        for (const auto& country : countries) {
            auto cIt = loadedData.find(country);
            if (cIt != loadedData.end()) {
                auto vIt = cIt->second.find(col);
                if (vIt != cIt->second.end()) series.values.push_back(vIt->second.asFloat());
                else series.values.push_back(0.0f);
            } else {
                series.values.push_back(0.0f);
            }
        }
        result.push_back(std::move(series));
    }
    return result;
}

void DataHandler::loadFromCSV(std::string file, std::vector<std::string> columns) {
    if (loadedFiles.count(file) > 0) return;
    if (!csvReader.load(file)) return;

    auto countries = csvReader.getColumnAsStrings("Country");
    for (size_t i = 0; i < countries.size(); ++i) {
        for (const auto& col : columns) {
            std::string rawValue = csvReader.getColumnAsStrings(col)[i];
            loadedData[countries[i]][col] = parseValueWithUnit(rawValue);
        }
    }
    loadedFiles.insert(file);
}

DataValue DataHandler::parseValueWithUnit(const std::string& rawValue) {
    if (rawValue.empty()) {
        return DataValue(0.0f, "");
    }

    // Remove commas for number parsing
    std::string cleaned = rawValue;
    cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), ','), cleaned.end());

    // Try to extract number and unit
    std::string numberPart;
    std::string unitPart;
    bool foundNumber = false;
    bool isNegative = false;

    for (size_t i = 0; i < cleaned.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(cleaned[i]);

        if (c == '-' && !foundNumber && numberPart.empty()) {
            isNegative = true;
            numberPart += c;
        } else if (std::isdigit(c) || c == '.') {
            numberPart += c;
            foundNumber = true;
        } else if (foundNumber) {
            // Everything after the number is the unit
            unitPart = cleaned.substr(i);
            // Trim leading whitespace from unit
            size_t start = unitPart.find_first_not_of(" \t");
            if (start != std::string::npos) {
                unitPart = unitPart.substr(start);
            }
            break;
        }
    }

    // If we found a valid number, parse it
    if (foundNumber && !numberPart.empty() && numberPart != "-") {
        try {
            float value = std::stof(numberPart);
            return DataValue(value, unitPart);
        } catch (...) {
            // If parsing fails, treat as string
            return DataValue(rawValue, "");
        }
    }

    // If no number found, store as string
    return DataValue(rawValue, "");
}

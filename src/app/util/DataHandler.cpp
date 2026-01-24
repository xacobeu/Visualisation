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
    loadFromCSV(DEMOGRAPHICS_CSV_PATH, {
        "Total_Population",
        "Population_Growth_Rate",
        "Birth_Rate",
        "Death_Rate",
        "Net_Migration_Rate",
        "Median_Age",
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
        
        // Extract unit from column name or from first data value with a unit
        for (const auto& country : countries) {
            auto cIt = loadedData.find(country);
            if (cIt != loadedData.end()) {
                auto vIt = cIt->second.find(col);
                if (vIt != cIt->second.end()) {
                    series.values.push_back(vIt->second.asFloat());
                    // Capture unit from the first entry that has one
                    if (series.unit.empty() && !vIt->second.unit.empty()) {
                        series.unit = vIt->second.unit;
                    }
                } else {
                    series.values.push_back(0.0f);
                }
            } else {
                series.values.push_back(0.0f);
            }
        }
        
        // If no unit found in data, try to extract from column name
        if (series.unit.empty()) {
            if (col.find("_billion_USD") != std::string::npos) {
                series.unit = "billion USD";
            } else if (col.find("_million_USD") != std::string::npos) {
                series.unit = "million USD";
            } else if (col.find("_USD") != std::string::npos) {
                series.unit = "USD";
            } else if (col.find("_percent") != std::string::npos) {
                series.unit = "%";
            } else if (col.find("_per_USD") != std::string::npos) {
                series.unit = "per USD";
            } else if (col.find("_sq_km") != std::string::npos) {
                series.unit = "sq km";
            } else if (col.find("_km") != std::string::npos) {
                series.unit = "km";
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

std::unordered_map<std::string, float> DataHandler::getColumnForAllCountries(const std::string& column) {
    std::unordered_map<std::string, float> result;
    
    for (const auto& [country, data] : loadedData) {
        auto it = data.find(column);
        if (it != data.end() && it->second.isNumeric()) {
            float val = it->second.asFloat();
            if (val > 0.0f) {  // Only include positive values
                result[country] = val;
            }
        }
    }
    
    return result;
}

DataHandler::ColumnData DataHandler::getColumnWithUnitForAllCountries(const std::string& column) {
    ColumnData result;
    
    for (const auto& [country, data] : loadedData) {
        auto it = data.find(column);
        if (it != data.end() && it->second.isNumeric()) {
            float val = it->second.asFloat();
            if (val > 0.0f) {  // Only include positive values
                result.values[country] = val;
                // Capture the unit from the first entry that has one
                if (result.unit.empty() && !it->second.unit.empty()) {
                    result.unit = it->second.unit;
                }
            }
        }
    }
    
    // If no unit was found in cell values, try to extract from column name
    // Common patterns: column_name_unit (e.g., "Real_GDP_PPP_billion_USD", "Budget_Surplus_billion_USD")
    if (result.unit.empty()) {
        // Look for common unit patterns in column name
        if (column.find("_billion_USD") != std::string::npos) {
            result.unit = "billion USD";
        } else if (column.find("_million_USD") != std::string::npos) {
            result.unit = "million USD";
        } else if (column.find("_USD") != std::string::npos) {
            result.unit = "USD";
        } else if (column.find("_percent") != std::string::npos) {
            result.unit = "%";
        } else if (column.find("_per_USD") != std::string::npos) {
            result.unit = "per USD";
        } else if (column.find("_km") != std::string::npos) {
            result.unit = "km";
        } else if (column.find("_sq_km") != std::string::npos) {
            result.unit = "sq km";
        } else if (column.find("_Age") != std::string::npos) {
            result.unit = "years";
        } else if (column.find("Sex_Ratio") != std::string::npos) {
            result.unit = "(Male : Female)";
        } else if (column.find("_kW") != std::string::npos) {
            result.unit = "kW";
        } else if (column.find("_bbl_per_day") != std::string::npos) {
            result.unit = "barrels per day";
        } else if (column.find("_cubic_meters") != std::string::npos) {
            result.unit = "cubic meters";
        } else if (column.find("_Mt") != std::string::npos) {
            result.unit = "million tons";
        }
    }
    
    return result;
}

std::vector<std::string> DataHandler::getAvailableColumns() const {
    std::unordered_set<std::string> columns;
    for (const auto& [country, data] : loadedData) {
        for (const auto& [col, val] : data) {
            if (val.isNumeric()) {
                columns.insert(col);
            }
        }
    }
    return std::vector<std::string>(columns.begin(), columns.end());
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

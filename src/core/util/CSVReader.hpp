#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <optional>

class CSVReader {
public:
    struct Row {
        std::unordered_map<std::string, std::string> data;

        std::optional<double> getDouble(const std::string& columnName) const {
            auto it = data.find(columnName);
            if (it == data.end() || it->second.empty()) {
                return std::nullopt;
            }
            try {
                return std::stod(it->second);
            } catch (...) {
                return std::nullopt;
            }
        }

        std::optional<float> getFloat(const std::string& columnName) const {
            auto it = data.find(columnName);
            if (it == data.end() || it->second.empty()) {
                return std::nullopt;
            }
            try {
                return std::stof(it->second);
            } catch (...) {
                return std::nullopt;
            }
        }

        std::string getString(const std::string& columnName) const {
            auto it = data.find(columnName);
            return (it != data.end()) ? it->second : "";
        }
    };

private:
    std::vector<std::string> headers;
    std::vector<Row> rows;

    static std::vector<std::string> parseLine(const std::string& line) {
        std::vector<std::string> result;
        std::string current;
        bool inQuotes = false;

        for (size_t i = 0; i < line.size(); ++i) {
            char c = line[i];

            if (c == '"') {
                inQuotes = !inQuotes;
            } else if (c == ',' && !inQuotes) {
                result.push_back(current);
                current.clear();
            } else {
                current += c;
            }
        }
        result.push_back(current);

        return result;
    }

public:
    bool load(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            return false;
        }

        rows.clear();
        headers.clear();

        std::string line;

        // Read header
        if (std::getline(file, line)) {
            headers = parseLine(line);
        } else {
            return false;
        }

        // Read data rows
        while (std::getline(file, line)) {
            if (line.empty()) continue;

            auto values = parseLine(line);
            if (values.size() != headers.size()) {
                continue; // Skip malformed rows
            }

            Row row;
            for (size_t i = 0; i < headers.size(); ++i) {
                row.data[headers[i]] = values[i];
            }
            rows.push_back(row);
        }

        return true;
    }

    const std::vector<std::string>& getHeaders() const {
        return headers;
    }

    const std::vector<Row>& getRows() const {
        return rows;
    }

    size_t rowCount() const {
        return rows.size();
    }

    // Helper function to extract a column as a vector of doubles (for ImPlot)
    std::vector<double> getColumnAsDoubles(const std::string& columnName) const {
        std::vector<double> result;
        result.reserve(rows.size());

        for (const auto& row : rows) {
            auto value = row.getDouble(columnName);
            result.push_back(value.value_or(0.0));
        }

        return result;
    }

    // Helper function to extract a column as a vector of floats (for ImPlot)
    std::vector<float> getColumnAsFloats(const std::string& columnName) const {
        std::vector<float> result;
        result.reserve(rows.size());

        for (const auto& row : rows) {
            auto value = row.getFloat(columnName);
            result.push_back(value.value_or(0.0f));
        }

        return result;
    }

    // Helper to get column as strings
    std::vector<std::string> getColumnAsStrings(const std::string& columnName) const {
        std::vector<std::string> result;
        result.reserve(rows.size());

        for (const auto& row : rows) {
            result.push_back(row.getString(columnName));
        }

        return result;
    }
};

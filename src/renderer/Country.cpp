#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>
#include "Country.hpp"

using json = nlohmann::json;

static std::vector<Vector2> parseRing(const json& ringArray) {
    std::vector<Vector2> ring;
    ring.reserve(ringArray.size());
    for (const auto& p : ringArray) {
        // p = [lon, lat]
        double lon = p[0].get<double>();
        double lat = p[1].get<double>();
        ring.push_back(Vector2{ float(lon), float(lat) });
    }
    return ring;
}

bool loadCountriesJson(const std::string& path, std::vector<Country>& out) {
    std::ifstream in(path);
    if (!in) {
        std::cerr << "Failed to open countries file: " << path << "\n";
        return false;
    }

    json j;
    try {
        in >> j;
    } catch (const std::exception& e) {
        std::cerr << "JSON parse error: " << e.what() << "\n";
        return false;
    }

    if (!j.is_array()) {
        std::cerr << "Expected top-level JSON array.\n";
        return false;
    }

    out.clear();
    out.reserve(j.size());

    for (const auto& c : j) {
        Country country;

        country.isoA3    = c.value("iso_a3",    "");
        country.name     = c.value("name",      "");
        country.nameLong = c.value("name_long", "");
        country.continent= c.value("continent", "");
        country.regionUN = c.value("region_un", "");
        country.subregion= c.value("subregion", "");

        const auto& borders = c["borders"];
        country.borders.reserve(borders.size());
        for (const auto& ring : borders) {
            country.borders.push_back(parseRing(ring));
        }

        out.push_back(std::move(country));
    }

    std::cout << "Loaded " << out.size() << " countries\n";
    return true;
}

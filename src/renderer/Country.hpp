// Countries.hpp
#pragma once

#include <string>
#include <vector>
#include "../util/MathUtils.hpp"

struct Country {
    std::string isoA3;
    std::string name;
    std::string nameLong;
    std::string continent;
    std::string regionUN;
    std::string subregion;
    std::vector<std::vector<Vector2>> borders;
};

bool loadCountriesJson(const std::string& path, std::vector<Country>& out);

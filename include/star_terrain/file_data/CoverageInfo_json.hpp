#pragma once

#include <nlohmann/json.hpp>

namespace star::terrain
{
struct CoverageInfo;
void to_json(nlohmann::json &j, const CoverageInfo &data);
void from_json(const nlohmann::json &j, CoverageInfo &data);
} // namespace star::terrain
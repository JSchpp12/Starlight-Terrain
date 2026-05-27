#include "star_terrain/file_data/coverage_info/CoverageInfo_json.hpp"
#include "star_terrain/file_data/coverage_info/CoverageInfo.hpp"

#include <starlight/core/json/glm_json.hpp>

namespace star::terrain
{
void to_json(nlohmann::json &j, const CoverageInfo &data)
{
    j["view_distance"] = data.viewDistance;
    j["center"] = data.center;
}

void from_json(const nlohmann::json &j, CoverageInfo &data)
{
    data.center = j["center"].get<glm::dvec2>();
    data.viewDistance = j["view_distance"].get<int>();
}
} // namespace star::terrain
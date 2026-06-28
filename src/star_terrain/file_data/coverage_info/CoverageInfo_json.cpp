#include "star_terrain/file_data/coverage_info/CoverageInfo_json.hpp"
#include "star_terrain/file_data/coverage_info/CoverageInfo.hpp"

#include <string>

namespace star::terrain
{
void to_json(nlohmann::json &j, const CoverageInfo &data)
{
    j["range"] = data.viewDistance;
    j["center"] = {{"lat", std::to_string(data.center.x)}, {"lon", std::to_string(data.center.y)}};
}

void from_json(const nlohmann::json &j, CoverageInfo &data)
{
    data.center = {std::stod(j["center"]["lat"].get<std::string>()),
                   std::stod(j["center"]["lon"].get<std::string>())};
    data.viewDistance = j["range"].get<int>();
}
} // namespace star::terrain
#include "star_terrain/file_data/CoverageInfo_json.hpp"
#include "star_terrain/file_data/CoverageInfo.hpp"

#include <starlight/core/Exceptions.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>

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
    try
    {
        data.center = {std::stod(j["center"]["lat"].get<std::string>()),
                       std::stod(j["center"]["lon"].get<std::string>())};
    }
    catch (const std::exception &e)
    {
        STAR_THROW_CAUSE("Failed to parse CoverageInfo.center", e);
    }

    try
    {
        data.viewDistance = j["range"].get<int>();
    }
    catch (const std::exception &e)
    {
        STAR_THROW_CAUSE("Failed to parse CoverageInfo.range", e);
    }
}
} // namespace star::terrain
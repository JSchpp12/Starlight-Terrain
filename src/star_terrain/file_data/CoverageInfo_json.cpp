#include "star_terrain/file_data/CoverageInfo_json.hpp"
#include "star_terrain/file_data/CoverageInfo.hpp"

#include <starlight/core/Exceptions.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>
#include <starlight/core/json/glm_json.hpp>

namespace star::terrain
{
void to_json(nlohmann::json &j, const CoverageInfo &data)
{
    j["view_distance"] = data.viewDistance;
    // Internally center.x = lat, center.y = lon. JSON convention: x = lon, y = lat.
    j["center"] = glm::dvec2{data.center.y, data.center.x};
}

void from_json(const nlohmann::json &j, CoverageInfo &data)
{
    try
    {
        const auto swapped = j["center"].get<glm::dvec2>();
        // JSON x = lon, y = lat -> internal center.x = lat, center.y = lon.
        data.center = glm::dvec2{swapped.y, swapped.x};
    }
    catch (const std::exception &e)
    {
        STAR_THROW_CAUSE("Failed to parse CoverageInfo.center", e);
    }

    try
    {
        data.viewDistance = j["view_distance"].get<int>();
    }
    catch (const std::exception &e)
    {
        STAR_THROW_CAUSE("Failed to parse CoverageInfo.view_distance", e);
    }
}
} // namespace star::terrain
#include "star_terrain/file_data/ChunkInfo_json.hpp"
#include "star_terrain/file_data/ChunkInfo.hpp"

#include <starlight/core/Exceptions.hpp>
#include <starlight/core/logging/LoggingFactory.hpp>

#include <string>

namespace star::terrain
{
void to_json(nlohmann::json &j, const ChunkInfo &d)
{
}

void from_json(const nlohmann::json &j, ChunkInfo &d)
{
    try
    {
        double lat = j["bounds"]["northEast"]["lat"];
        double lon = j["bounds"]["northEast"]["lon"];
        d.cornerNE = glm::dvec2{lat, lon};
    }
    catch (const std::exception &e)
    {
        STAR_THROW_CAUSE("Failed to parse ChunkInfo.bounds.northEast", e);
    }

    try
    {
        double lat = j["bounds"]["southEast"]["lat"];
        double lon = j["bounds"]["southEast"]["lon"];
        d.cornerSE = glm::dvec2{lat, lon};
    }
    catch (const std::exception &e)
    {
        STAR_THROW_CAUSE("Failed to parse ChunkInfo.bounds.southEast", e);
    }

    try
    {
        double lat = j["bounds"]["southWest"]["lat"];
        double lon = j["bounds"]["southWest"]["lon"];
        d.cornerSW = glm::dvec2{lat, lon};
    }
    catch (const std::exception &e)
    {
        STAR_THROW_CAUSE("Failed to parse ChunkInfo.bounds.southWest", e);
    }

    try
    {
        double lat = j["bounds"]["northWest"]["lat"];
        double lon = j["bounds"]["northWest"]["lon"];
        d.cornerNW = glm::dvec2{lat, lon};
    }
    catch (const std::exception &e)
    {
        STAR_THROW_CAUSE("Failed to parse ChunkInfo.bounds.northWest", e);
    }

    try
    {
        double lat = j["bounds"]["center"]["lat"];
        double lon = j["bounds"]["center"]["lon"];
        d.center = glm::dvec2{lat, lon};
    }
    catch (const std::exception &e)
    {
        STAR_THROW_CAUSE("Failed to parse ChunkInfo.bounds.center", e);
    }

    try
    {
        d.textureFile = j["name"];
    }
    catch (const std::exception &e)
    {
        STAR_THROW_CAUSE("Failed to parse ChunkInfo.name", e);
    }
}
} // namespace star::terrain

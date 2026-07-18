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

static std::tuple<bool, double, double> TryGetBounds(const nlohmann::json &j) noexcept
{
    bool success{true};
    double lat{0}, lon{0};

    try
    {
        lat = j["lat"];
        lon = j["lon"];
    }
    catch (const std::exception &e)
    {
        try
        {
            const std::string sLat = j["lat"];
            lat = std::stod(sLat);

            const std::string sLon = j["lon"];
            lon = std::stod(sLon);
        }
        catch (const std::exception &e)
        {
            success = false;
        }
    }

    return std::make_tuple(success, lat, lon);
}

void from_json(const nlohmann::json &j, ChunkInfo &d)
{
    double lat, lon;

    {
        auto [success, lat, lon] = TryGetBounds(j["bounds"]["northEast"]);
        if (!success)
            STAR_THROW("Failed to parse ChunkInfo.bounds.northEast");
        d.cornerNE = glm::dvec2{lat, lon};
    }

    {
        auto [success, lat, lon] = TryGetBounds(j["bounds"]["southEast"]);
        if (!success)
            STAR_THROW("Failed to parse ChunkInfo.bounds.southEast");
        d.cornerSE = glm::dvec2{lat, lon};
    }

    {
        auto [success, lat, lon] = TryGetBounds(j["bounds"]["southWest"]);
        if (!success)
            STAR_THROW("Failed to parse ChunkInfo.bounds.southWest");
        d.cornerSW = glm::dvec2{lat, lon};
    }

    {
        auto [success, lat, lon] = TryGetBounds(j["bounds"]["northWest"]);
        if (!success)
            STAR_THROW("Failed to parse ChunkInfo.bounds.northWest");
        d.cornerNW = glm::dvec2{lat, lon};
    }

    {
        auto [success, lat, lon] = TryGetBounds(j["bounds"]["center"]);
        if (!success)
            STAR_THROW("Failed to parse ChunkInfo.bounds.center");
        d.center = glm::dvec2{lat, lon};
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

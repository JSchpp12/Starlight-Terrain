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

static bool TryReadCoordinate(const nlohmann::json &node, const char *key, double &out) noexcept
{
    const auto it = node.find(key);
    if (it == node.end())
    {
        return false;
    }

    try
    {
        if (it->is_number())
        {
            out = it->get<double>();
            return true;
        }

        if (it->is_string())
        {
            out = std::stod(it->get<std::string>());
            return true;
        }
    }
    catch (const std::exception &)
    {
        return false;
    }

    return false;
}

static std::tuple<bool, double, double> TryGetBounds(const nlohmann::json &j) noexcept
{
    double lat{0}, lon{0};
    const bool success = TryReadCoordinate(j, "lat", lat) && TryReadCoordinate(j, "lon", lon);
    return std::make_tuple(success, lat, lon);
}

static glm::dvec2 ReadBoundsCorner(const nlohmann::json &j, const char *corner)
{
    const auto boundsIt = j.find("bounds");
    if (boundsIt == j.end() || !boundsIt->is_object())
    {
        STAR_THROW(std::string{"ChunkInfo is missing the 'bounds' object required for corner '"} + corner + "'");
    }

    const auto cornerIt = boundsIt->find(corner);
    if (cornerIt == boundsIt->end())
    {
        STAR_THROW(std::string{"ChunkInfo.bounds is missing corner '"} + corner + "'");
    }

    const auto [success, lat, lon] = TryGetBounds(*cornerIt);
    if (!success)
    {
        STAR_THROW(std::string{"Failed to parse ChunkInfo.bounds."} + corner +
                   " (expected numeric or numeric-string lat/lon)");
    }

    return glm::dvec2{lat, lon};
}

void from_json(const nlohmann::json &j, ChunkInfo &d)
{
    d.cornerNE = ReadBoundsCorner(j, "northEast");
    d.cornerSE = ReadBoundsCorner(j, "southEast");
    d.cornerSW = ReadBoundsCorner(j, "southWest");
    d.cornerNW = ReadBoundsCorner(j, "northWest");
    d.center = ReadBoundsCorner(j, "center");

    try
    {
        d.textureFile = j.at("name").get<std::string>();
    }
    catch (const std::exception &e)
    {
        STAR_THROW_CAUSE("Failed to parse ChunkInfo.name", e);
    }
}
} // namespace star::terrain

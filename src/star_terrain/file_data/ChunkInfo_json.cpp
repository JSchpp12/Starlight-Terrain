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
        d.cornerNE = glm::dvec2{std::stod(j["bounds"]["northEast"]["lat"].get<std::string>()),
                                std::stod(j["bounds"]["northEast"]["lon"].get<std::string>())};
    }
    catch (const std::exception &e)
    {
        STAR_THROW_CAUSE("Failed to parse ChunkInfo.bounds.northEast", e);
    }

    try
    {
        d.cornerSE = glm::dvec2{std::stod(j["bounds"]["southEast"]["lat"].get<std::string>()),
                                std::stod(j["bounds"]["southEast"]["lon"].get<std::string>())};
    }
    catch (const std::exception &e)
    {
        STAR_THROW_CAUSE("Failed to parse ChunkInfo.bounds.southEast", e);
    }

    try
    {
        d.cornerSW = glm::dvec2{std::stod(j["bounds"]["southWest"]["lat"].get<std::string>()),
                                std::stod(j["bounds"]["southWest"]["lon"].get<std::string>())};
    }
    catch (const std::exception &e)
    {
        STAR_THROW_CAUSE("Failed to parse ChunkInfo.bounds.southWest", e);
    }

    try
    {
        d.cornerNW = glm::dvec2{std::stod(j["bounds"]["northWest"]["lat"].get<std::string>()),
                                std::stod(j["bounds"]["northWest"]["lon"].get<std::string>())};
    }
    catch (const std::exception &e)
    {
        STAR_THROW_CAUSE("Failed to parse ChunkInfo.bounds.northWest", e);
    }

    try
    {
        d.center = glm::dvec2{std::stod(j["bounds"]["center"]["lat"].get<std::string>()),
                              std::stod(j["bounds"]["center"]["lon"].get<std::string>())};
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

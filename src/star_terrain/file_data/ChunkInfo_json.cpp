#include "star_terrain/file_data/ChunkInfo_json.hpp"
#include "star_terrain/file_data/ChunkInfo.hpp"

#include <string>

namespace star::terrain
{
void to_json(nlohmann::json &j, const ChunkInfo &d)
{
}

void from_json(const nlohmann::json &j, ChunkInfo &d)
{
    d.cornerNE = glm::dvec2{std::stod(j["bounds"]["northEast"]["lat"].get<std::string>()),
                            std::stod(j["bounds"]["northEast"]["lon"].get<std::string>())};
    d.cornerSE = glm::dvec2{std::stod(j["bounds"]["southEast"]["lat"].get<std::string>()),
                            std::stod(j["bounds"]["southEast"]["lon"].get<std::string>())};
    d.cornerSW = glm::dvec2{std::stod(j["bounds"]["southWest"]["lat"].get<std::string>()),
                             std::stod(j["bounds"]["southWest"]["lon"].get<std::string>())};
    d.cornerNW = glm::dvec2{std::stod(j["bounds"]["northWest"]["lat"].get<std::string>()),
                            std::stod(j["bounds"]["northWest"]["lon"].get<std::string>())};
    d.center = glm::dvec2{std::stod(j["bounds"]["center"]["lat"].get<std::string>()),
                           std::stod(j["bounds"]["center"]["lon"].get<std::string>())};
    d.textureFile = j["name"];
}
} // namespace star::terrain
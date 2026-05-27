#pragma once

#include <glm/glm.hpp>
#include <string>
#include <vector>

class TerrainInfoFile
{
  public:
    struct TerrainInfo
    {
        glm::dvec2 cornerNE, cornerSE, cornerSW, cornerNW, center;
        std::string textureFile;

        TerrainInfo(const glm::dvec2 &cornerNE, const glm::dvec2 &cornerSE, const glm::dvec2 &cornerSW,
                    const glm::dvec2 &cornerNW, const glm::dvec2 &center, const std::string &textureFile)
            : cornerNE(cornerNE), cornerSE(cornerSE), cornerSW(cornerSW), cornerNW(cornerNW), center(center),
              textureFile(textureFile)
        {
        }

        TerrainInfo() = default;
    };

    TerrainInfoFile(const std::string &pathToFile);

    const std::vector<TerrainInfo> &infos() const
    {
        return this->parsedInfo;
    }

    const std::string getFullHeightFilePath() const
    {
        return this->fullHeightFilePath;
    }

  private:
    std::string fullHeightFilePath;

    /// @brief Load terrain info from json file
    /// @param pathToFile full path to the file to be read
    void loadFromFile(const std::string &pathToFile);

    std::vector<TerrainInfo> parsedInfo = std::vector<TerrainInfo>();
};
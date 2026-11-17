#include "star_terrain/io/TerrainLibraryEntryReader.hpp"

#include <starlight/core/logging/LoggingFactory.hpp>

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace star::terrain
{
static constexpr const char *kShapeFileName = "Shape.json";
static constexpr const char *kHeightInfoFileName = "height_info.json";

TerrainLibrary TerrainLibraryEntryReader::load(const std::filesystem::path &terrainRoot) const
{
    std::error_code error;
    if (!std::filesystem::is_directory(terrainRoot, error))
        throw std::runtime_error("Terrain library path is not a directory: " + terrainRoot.string());

    if (!std::filesystem::is_regular_file(terrainRoot / kHeightInfoFileName, error))
        throw std::runtime_error("Terrain folder has no " + std::string{kHeightInfoFileName} + ": " +
                                 terrainRoot.string());

    if (std::filesystem::is_regular_file(terrainRoot / kShapeFileName, error))
        return TerrainLibrary{.terrainRootPath = terrainRoot.string(), .terrainShapeFile = kShapeFileName};

    // Fall back to the single shape descriptor when the folder does not use the conventional name. Info sidecars
    // (height_info.json, elevation_merged_info.json) are not descriptors.
    std::vector<std::string> candidates;
    for (std::filesystem::directory_iterator it(terrainRoot, error), end; it != end; it.increment(error))
    {
        if (error)
            throw std::runtime_error("Failed to enumerate '" + terrainRoot.string() + "': " + error.message());

        std::error_code fileError;
        if (!it->is_regular_file(fileError))
            continue;
        if (it->path().extension() != ".json")
            continue;

        const std::string filename = it->path().filename().string();
        if (filename == kHeightInfoFileName || filename.ends_with("_info.json"))
            continue;

        candidates.push_back(filename);
    }

    if (error)
        throw std::runtime_error("Failed to enumerate '" + terrainRoot.string() + "': " + error.message());

    if (candidates.empty())
        throw std::runtime_error("No terrain shape descriptor found in " + terrainRoot.string());

    if (candidates.size() > 1)
    {
        std::sort(candidates.begin(), candidates.end());
        std::string list;
        for (const auto &candidate : candidates)
        {
            if (!list.empty())
                list += ", ";
            list += candidate;
        }
        throw std::runtime_error("Ambiguous terrain shape descriptors in " + terrainRoot.string() + ": " + list);
    }

    return TerrainLibrary{.terrainRootPath = terrainRoot.string(), .terrainShapeFile = candidates.front()};
}

int TerrainLibraryEntryReader::operator()(const std::filesystem::path &terrainRoot)
{
    try
    {
        TerrainLibrary library = load(terrainRoot);
        star::core::logging::debug("Resolved terrain library '", library.terrainRootPath, "' with shape descriptor '",
                                   library.terrainShapeFile, "'");
        m_result.set_value(std::move(library));
    }
    catch (const std::exception &e)
    {
        star::core::logging::warning("Failed to resolve terrain library entry '", terrainRoot.string(),
                                     "': ", e.what());
        m_result.set_exception(std::current_exception());
    }
    catch (...)
    {
        star::core::logging::warning("Failed to resolve terrain library entry '", terrainRoot.string(),
                                     "': unknown error");
        m_result.set_exception(std::current_exception());
    }

    return 0;
}
} // namespace star::terrain

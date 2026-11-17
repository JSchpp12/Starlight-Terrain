#include "star_terrain/io/TerrainLibraryReader.hpp"

#include <starlight/core/logging/LoggingFactory.hpp>

#include <algorithm>
#include <exception>
#include <stdexcept>
#include <utility>

namespace star::terrain
{
std::vector<TerrainLibraryScanEntry> TerrainLibraryReader::scan(const std::filesystem::path &directory) const
{
    std::error_code error;
    std::vector<TerrainLibraryScanEntry> entries;
    for (std::filesystem::directory_iterator it(directory, error), end; it != end; it.increment(error))
    {
        if (error)
        {
            throw std::runtime_error("Failed to enumerate '" + directory.string() + "': " + error.message());
        }

        std::error_code typeError;
        if (!it->is_directory(typeError))
            continue;

        std::error_code markerError;
        if (!std::filesystem::is_regular_file(it->path() / "height_info.json", markerError))
            continue;

        entries.push_back(
            TerrainLibraryScanEntry{.displayName = it->path().filename().string(), .terrainRootPath = it->path()});
        star::core::logging::debug("Terrain library entry: '", it->path().string(), "'");
    }

    if (error)
    {
        throw std::runtime_error("Failed to enumerate '" + directory.string() + "': " + error.message());
    }

    std::sort(entries.begin(), entries.end(),
              [](const TerrainLibraryScanEntry &lhs, const TerrainLibraryScanEntry &rhs) {
                  if (lhs.displayName != rhs.displayName)
                      return lhs.displayName < rhs.displayName;
                  return lhs.terrainRootPath.filename().string() < rhs.terrainRootPath.filename().string();
              });

    return entries;
}

int TerrainLibraryReader::operator()(const std::filesystem::path &directory)
{
    TerrainLibraryScanResult result = [&]() -> TerrainLibraryScanResult {
        try
        {
            auto entries = scan(directory);
            star::core::logging::info("Terrain library scan found ", entries.size(), " entr(ies) in ",
                                      directory.string());
            return TerrainLibraryScanResult{std::move(entries)};
        }
        catch (const std::exception &e)
        {
            star::core::logging::warning("Terrain library scan failed for '", directory.string(), "': ", e.what());
            return std::unexpected("Terrain library scan failed: " + std::string(e.what()));
        }
        catch (...)
        {
            star::core::logging::warning("Terrain library scan failed for '", directory.string(), "': unknown error");
            return std::unexpected(std::string{"Terrain library scan failed: unknown error"});
        }
    }();

    m_result.set_value(std::move(result));
    return 0;
}
} // namespace star::terrain

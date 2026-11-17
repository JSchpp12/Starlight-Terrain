#pragma once

#include "star_terrain/io/TerrainLibrary.hpp"

#include <filesystem>
#include <future>

namespace star::terrain
{
/// @brief Resolves one terrain folder into a TerrainLibrary record by discovering its shape descriptor.
class TerrainLibraryEntryReader
{
  public:
    /// @brief Resolves `terrainRoot` and stores the record.
    /// @return 0 always.
    int operator()(const std::filesystem::path &terrainRoot);

    /// @brief Future resolving once the read task runs. Call before moving the reader.
    [[nodiscard]] std::future<TerrainLibrary> getFuture()
    {
        return m_result.get_future();
    }

  private:
    [[nodiscard]] TerrainLibrary load(const std::filesystem::path &terrainRoot) const;

    std::promise<TerrainLibrary> m_result;
};
} // namespace star::terrain

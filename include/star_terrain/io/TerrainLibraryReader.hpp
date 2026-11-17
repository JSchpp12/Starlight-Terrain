#pragma once

#include <expected>
#include <filesystem>
#include <future>
#include <string>
#include <vector>

namespace star::terrain
{
/// @brief One terrain folder discovered by the root scan; its shape descriptor is resolved lazily.
struct TerrainLibraryScanEntry
{
    std::string displayName;
    std::filesystem::path terrainRootPath;
};

/// @brief Result of the root scan: the discovered terrain folders or a human-readable failure reason.
using TerrainLibraryScanResult = std::expected<std::vector<TerrainLibraryScanEntry>, std::string>;

/// @brief Scans a resolved directory for immediate child terrain folders.
/// @details A folder qualifies as a terrain when it contains `height_info.json`.
class TerrainLibraryReader
{
  public:
    /// @brief Scans `directory` (already resolved by the payload) and stores the result.
    /// @return 0 always; failures travel through the future.
    int operator()(const std::filesystem::path &directory);

    /// @brief Future resolving once the scan task runs. Call before moving the reader.
    [[nodiscard]] std::future<TerrainLibraryScanResult> getFuture()
    {
        return m_result.get_future();
    }

  private:
    /// @brief Enumerates the directory. Throws on filesystem errors; operator() publishes the failure.
    [[nodiscard]] std::vector<TerrainLibraryScanEntry> scan(const std::filesystem::path &directory) const;

    std::promise<TerrainLibraryScanResult> m_result;
};
} // namespace star::terrain

#pragma once

#include "star_terrain/io/TerrainLibrary.hpp"

#include <future>
#include <utility>

namespace star::terrain
{
/// @brief Handle to an in-flight per-entry TerrainLibrary read. Move-only; consumed once by the service.
class PendingTerrainLibrary
{
  public:
    PendingTerrainLibrary() = default;
    explicit PendingTerrainLibrary(std::future<TerrainLibrary> future) : m_future(std::move(future))
    {
    }
    PendingTerrainLibrary(PendingTerrainLibrary &&) = default;
    PendingTerrainLibrary &operator=(PendingTerrainLibrary &&) = default;
    PendingTerrainLibrary(const PendingTerrainLibrary &) = delete;
    PendingTerrainLibrary &operator=(const PendingTerrainLibrary &) = delete;
    ~PendingTerrainLibrary() = default;

    [[nodiscard]] bool valid() const noexcept
    {
        return m_future.valid();
    }

    /// @brief Blocks until the IO worker resolves the record; rethrows the reader's exception on failure.
    [[nodiscard]] TerrainLibrary get()
    {
        return m_future.get();
    }

  private:
    std::future<TerrainLibrary> m_future;
};
} // namespace star::terrain

#pragma once

#include "star_terrain/file_data/ChunkInfo.hpp"
#include "star_terrain/io/TerrainTargetInfo.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace star::terrain
{
class IFilter;

/// @brief Owns the target chunks collected from the terrain library.
/// @details Non-copyable and non-movable: the cart is built in place and narrowed in place by filters.
class LibraryCart
{
  public:
    /// @param info Terrain-wide data shared by the chunks.
    /// @param chunks The target chunks collected for this terrain.
    LibraryCart(TerrainTargetInfo info, std::vector<ChunkInfo> chunks);
    LibraryCart(const LibraryCart &) = delete;
    LibraryCart &operator=(const LibraryCart &) = delete;
    LibraryCart(LibraryCart &&) = delete;
    LibraryCart &operator=(LibraryCart &&) = delete;
    ~LibraryCart() = default;

    /// @brief Drop every chunk rejected by a filter. A chunk is kept only when every filter keeps it.
    void applyFilters(const std::vector<const IFilter *> &filters);

    /// @brief Keep only the chunks at the given indices, preserving their existing order.
    /// @details Used after loading to drop targets that produced no mesh, so the cart stays the source of truth
    /// for what the terrain object contains.
    void retainOnly(std::span<const std::size_t> indices);

    [[nodiscard]] const TerrainTargetInfo &info() const
    {
        return m_info;
    }

    [[nodiscard]] const std::vector<ChunkInfo> &chunks() const
    {
        return m_chunks;
    }

  private:
    TerrainTargetInfo m_info;
    std::vector<ChunkInfo> m_chunks;
};
} // namespace star::terrain

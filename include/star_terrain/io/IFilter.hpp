#pragma once

#include "star_terrain/file_data/ChunkInfo.hpp"

namespace star::terrain
{
/// @brief Predicate deciding whether a chunk should stay in a LibraryCart.
class IFilter
{
  public:
    virtual ~IFilter() = default;

    /// @brief Returns true when the chunk passes this filter.
    [[nodiscard]] virtual bool shouldKeep(const ChunkInfo &chunk) const = 0;
};
} // namespace star::terrain

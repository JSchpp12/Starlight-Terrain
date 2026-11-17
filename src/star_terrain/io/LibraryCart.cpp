#include "star_terrain/io/LibraryCart.hpp"

#include "star_terrain/io/IFilter.hpp"

#include <utility>

namespace star::terrain
{
LibraryCart::LibraryCart(TerrainTargetInfo info, std::vector<ChunkInfo> chunks)
    : m_info(std::move(info)), m_chunks(std::move(chunks))
{
}

void LibraryCart::applyFilter(const IFilter *filter)
{
    if (filter == nullptr)
        return;

    std::vector<ChunkInfo> working;
    working.reserve(m_chunks.size());

    for (auto &chunk : m_chunks)
    {
        if (filter != nullptr && filter->shouldKeep(chunk))
        {
            working.push_back(std::move(chunk));
        }
    }

    m_chunks = std::move(working);
}
} // namespace star::terrain

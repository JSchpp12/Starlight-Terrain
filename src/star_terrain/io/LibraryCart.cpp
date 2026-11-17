#include "star_terrain/io/LibraryCart.hpp"

#include "star_terrain/io/IFilter.hpp"

#include <cassert>
#include <utility>

namespace star::terrain
{
LibraryCart::LibraryCart(TerrainTargetInfo info, std::vector<ChunkInfo> chunks)
    : m_info(std::move(info)), m_chunks(std::move(chunks))
{
}

void LibraryCart::applyFilters(const std::vector<const IFilter *> &filters)
{
    if (filters.empty())
        return;

    std::vector<ChunkInfo> working;
    working.reserve(m_chunks.size());

    for (auto &chunk : m_chunks)
    {
        bool keep = true;
        for (const IFilter *filter : filters)
        {
            if (filter != nullptr && !filter->shouldKeep(chunk))
            {
                keep = false;
                break;
            }
        }

        if (keep)
            working.push_back(std::move(chunk));
    }

    m_chunks = std::move(working);
}

void LibraryCart::retainOnly(std::span<const std::size_t> indices)
{
    std::vector<ChunkInfo> working;
    working.reserve(indices.size());

    for (const std::size_t index : indices)
    {
        assert(index < m_chunks.size() && "LibraryCart::retainOnly index out of range");
        working.push_back(std::move(m_chunks[index]));
    }

    m_chunks = std::move(working);
}
} // namespace star::terrain

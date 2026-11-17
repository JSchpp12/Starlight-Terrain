#pragma once

#include "star_terrain/io/CartReadException.hpp"
#include "star_terrain/io/IFilter.hpp"
#include "star_terrain/io/PendingLibraryCart.hpp"

#include <star_common/Handle.hpp>
#include <star_common/IServiceCommandWithReply.hpp>

#include <expected>
#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace star::terrain::command
{
namespace get_library_cart_for_terrain
{
inline constexpr const char *GetTypeName()
{
    return "trGLCT";
}
} // namespace get_library_cart_for_terrain

using GetLibraryCartForTerrainResult = std::expected<PendingLibraryCart, CartReadException>;

/// @brief Start reading a library cart for one terrain. The reply carries the pending cart immediately.
struct GetLibraryCartForTerrain : public star::common::IServiceCommandWithReply<GetLibraryCartForTerrainResult>
{
    static constexpr std::string_view GetUniqueTypeName()
    {
        return get_library_cart_for_terrain::GetTypeName();
    }

    GetLibraryCartForTerrain() = default;

    GetLibraryCartForTerrain &setHandle(star::Handle handle)
    {
        this->handle = std::move(handle);
        return *this;
    }

    GetLibraryCartForTerrain &setFilters(std::vector<std::unique_ptr<star::terrain::IFilter>> filters)
    {
        this->filters = std::move(filters);
        return *this;
    }

    star::Handle handle{};
    std::vector<std::unique_ptr<star::terrain::IFilter>> filters;
};
} // namespace star::terrain::command

#pragma once

#include "star_terrain/io/CartReadException.hpp"
#include "star_terrain/io/PendingLibraryCart.hpp"

#include <star_common/Handle.hpp>
#include <star_common/IServiceCommandWithReply.hpp>

#include <cstdint>
#include <expected>
#include <string_view>
#include <utility>

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
    explicit GetLibraryCartForTerrain(star::Handle handle) : handle(std::move(handle))
    {
    }
    GetLibraryCartForTerrain(star::Handle handle, uint16_t type)
        : star::common::IServiceCommandWithReply<GetLibraryCartForTerrainResult>(type), handle(std::move(handle))
    {
    }

    star::Handle handle{};
};
} // namespace star::terrain::command

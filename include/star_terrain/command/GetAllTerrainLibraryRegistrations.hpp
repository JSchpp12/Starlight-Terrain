#pragma once

#include "star_terrain/command/LibraryRegistrationTypes.hpp"

#include <star_common/IServiceCommandWithReply.hpp>

#include <cstdint>
#include <string_view>

namespace star::terrain::command
{
namespace get_all_terrain_library_registrations
{
inline constexpr const char *GetTypeName()
{
    return "trGTLR";
}
} // namespace get_all_terrain_library_registrations

/// @brief Resolve every library entry and return the registrations. Blocks until the library scan and every per-entry
/// record read are complete.
struct GetAllTerrainLibraryRegistrations
    : public star::common::IServiceCommandWithReply<GetAllTerrainLibraryRegistrationsReply>
{
    static constexpr std::string_view GetUniqueTypeName()
    {
        return get_all_terrain_library_registrations::GetTypeName();
    }

    GetAllTerrainLibraryRegistrations() = default;
    explicit GetAllTerrainLibraryRegistrations(uint16_t type)
        : star::common::IServiceCommandWithReply<GetAllTerrainLibraryRegistrationsReply>(type)
    {
    }
};
} // namespace star::terrain::command

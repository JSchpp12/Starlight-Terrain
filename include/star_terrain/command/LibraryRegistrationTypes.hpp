#pragma once

#include <star_common/Handle.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace star::terrain::command
{
/// @brief One terrain library entry as surfaced to callers.
/// @details `error` is empty on success; `terrainShapeFile` is only meaningful when the entry resolved.
struct TerrainLibraryRegistration
{
    star::Handle handle{};
    std::string displayName;
    std::filesystem::path terrainRootPath;
    std::string terrainShapeFile;
    std::string error;
};

/// @brief Every terrain library entry plus the root-scan failure, if any.
struct GetAllTerrainLibraryRegistrationsReply
{
    std::vector<TerrainLibraryRegistration> registrations;
    std::optional<std::string> scanError;
};
} // namespace star::terrain::command

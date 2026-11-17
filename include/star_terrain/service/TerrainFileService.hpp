#pragma once

#include "star_terrain/command/GetAllTerrainLibraryRegistrations.hpp"
#include "star_terrain/command/GetLibraryCartForTerrain.hpp"
#include "star_terrain/io/PendingTerrainLibrary.hpp"
#include "star_terrain/io/TerrainLibrary.hpp"
#include "star_terrain/io/TerrainLibraryReader.hpp"

#include <absl/container/flat_hash_map.h>
#include <star_common/Handle.hpp>
#include <starlight/policy/command/ListenFor.hpp>

#include <cstdint>
#include <expected>
#include <filesystem>
#include <future>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace star::core
{
class CommandBus;
class WorkerPool;
} // namespace star::core

namespace star::job
{
class TaskManager;
} // namespace star::job

namespace star::service
{
struct InitParameters;
} // namespace star::service

namespace star::terrain::service
{
template <typename T>
using ListenForGetAllTerrainLibraryRegistrations =
    star::policy::command::ListenFor<T, command::GetAllTerrainLibraryRegistrations,
                                     command::get_all_terrain_library_registrations::GetTypeName,
                                     &T::onGetAllTerrainLibraryRegistrations>;

template <typename T>
using ListenForGetLibraryCartForTerrain =
    star::policy::command::ListenFor<T, command::GetLibraryCartForTerrain,
                                     command::get_library_cart_for_terrain::GetTypeName,
                                     &T::onGetLibraryCartForTerrain>;

/// @brief Owns the terrain library scan and every entry's library record.
class TerrainFileService
{
  public:
    /// @param configuredTerrainDir The terrain directory from the app config. The library root is derived from its
    /// parent directory.
    explicit TerrainFileService(std::string configuredTerrainDir);
    TerrainFileService(const TerrainFileService &) = delete;
    TerrainFileService &operator=(const TerrainFileService &) = delete;
    TerrainFileService(TerrainFileService &&other);
    TerrainFileService &operator=(TerrainFileService &&other);
    ~TerrainFileService() = default;

    void setInitParameters(star::service::InitParameters &params);

    void negotiateWorkers(star::core::WorkerPool &, star::job::TaskManager &)
    {
    }

    void init();

    void shutdown();

    void onGetAllTerrainLibraryRegistrations(command::GetAllTerrainLibraryRegistrations &cmd);
    void onGetLibraryCartForTerrain(command::GetLibraryCartForTerrain &cmd);

  private:
    struct Registration
    {
        star::Handle handle{};
        std::string displayName;
        std::filesystem::path terrainRootPath;
        std::optional<std::string> error;
    };

    using EntryState = std::variant<PendingTerrainLibrary, TerrainLibrary>;

    void ensureLibraryBuilt();
    star::Handle addEntry(std::string displayName, std::filesystem::path terrainRootPath);
    Registration *registrationFor(star::Handle handle);
    std::expected<TerrainLibrary, std::string> resolveEntry(star::Handle handle);
    void initListeners(star::core::CommandBus &bus);
    void cleanupListeners(star::core::CommandBus &bus);

    std::string m_configuredTerrainDir;
    std::future<TerrainLibraryScanResult> m_libraryFuture;
    std::vector<Registration> m_registrations;
    absl::flat_hash_map<star::Handle, EntryState, star::HandleHash> m_libraries;
    std::optional<std::string> m_scanError;
    bool m_libraryBuilt{false};
    star::core::CommandBus *m_cmd{nullptr};
    ListenForGetAllTerrainLibraryRegistrations<TerrainFileService> m_onGetAll;
    ListenForGetLibraryCartForTerrain<TerrainFileService> m_onGetCart;
};
} // namespace star::terrain::service

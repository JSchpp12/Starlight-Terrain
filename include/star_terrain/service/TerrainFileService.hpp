#pragma once

#include <string>

namespace star::core
{
class CommandBus;
class WorkerPool;
} // namespace star::core

namespace star::core::device
{
class DeviceContext;
} // namespace star::core::device

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
/// @brief Owner of terrain file information
class TerrainFileService
{
  public:
    /// @param configuredTerrainDir The terrain directory from the app config.
    explicit TerrainFileService(std::string configuredTerrainDir);
    TerrainFileService(const TerrainFileService &) = delete;
    TerrainFileService &operator=(const TerrainFileService &) = delete;
    TerrainFileService(TerrainFileService &&other) = default;
    TerrainFileService &operator=(TerrainFileService &&other) = default;
    ~TerrainFileService() = default;

    void setInitParameters(star::service::InitParameters &params);
    void negotiateWorkers(star::core::WorkerPool &, star::job::TaskManager &)
    {
    }
    void init();
    void shutdown();

  private:
    [[maybe_unused]] std::string m_configuredTerrainDir;
    [[maybe_unused]] star::core::CommandBus *m_cmd{nullptr};
};
} // namespace star::terrain::service

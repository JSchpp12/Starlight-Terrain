#include "star_terrain/service/TerrainFileService.hpp"

#include "star_terrain/io/LibraryCartReader.hpp"
#include "star_terrain/io/PendingLibraryCart.hpp"
#include "star_terrain/io/TerrainLibraryEntryReader.hpp"

#include <star_common/HandleTypeRegistry.hpp>
#include <starlight/command/FileIO/ReadFromFile.hpp>
#include <starlight/core/CommandBus.hpp>
#include <starlight/job/tasks/IOTask.hpp>
#include <starlight/service/InitParameters.hpp>

#include <algorithm>
#include <cassert>
#include <exception>
#include <utility>

namespace star::terrain::service
{
static std::uint16_t TerrainLibraryHandleType()
{
    static const std::uint16_t type = star::common::HandleTypeRegistry::instance().registerType("stTerrainLibrary");
    return type;
}

TerrainFileService::TerrainFileService(std::string configuredTerrainDir)
    : m_configuredTerrainDir(std::move(configuredTerrainDir)), m_onGetAll(*this), m_onGetCart(*this)
{
}

TerrainFileService::TerrainFileService(TerrainFileService &&other)
    : m_configuredTerrainDir(std::move(other.m_configuredTerrainDir)),
      m_libraryFuture(std::move(other.m_libraryFuture)), m_registrations(std::move(other.m_registrations)),
      m_libraries(std::move(other.m_libraries)), m_scanError(std::move(other.m_scanError)),
      m_libraryBuilt(other.m_libraryBuilt), m_cmd(other.m_cmd), m_onGetAll(*this), m_onGetCart(*this)
{
    other.m_cmd = nullptr;

    if (m_cmd != nullptr)
    {
        other.cleanupListeners(*m_cmd);
        initListeners(*m_cmd);
    }
}

TerrainFileService &TerrainFileService::operator=(TerrainFileService &&other)
{
    if (this == &other)
        return *this;

    if (m_cmd != nullptr)
        cleanupListeners(*m_cmd);

    m_configuredTerrainDir = std::move(other.m_configuredTerrainDir);
    m_libraryFuture = std::move(other.m_libraryFuture);
    m_registrations = std::move(other.m_registrations);
    m_libraries = std::move(other.m_libraries);
    m_scanError = std::move(other.m_scanError);
    m_libraryBuilt = other.m_libraryBuilt;
    m_cmd = other.m_cmd;

    other.m_cmd = nullptr;

    if (m_cmd != nullptr)
    {
        other.cleanupListeners(*m_cmd);
        initListeners(*m_cmd);
    }

    return *this;
}

void TerrainFileService::setInitParameters(star::service::InitParameters &params)
{
    m_cmd = &params.commandBus;
}

void TerrainFileService::init()
{
    assert(m_cmd != nullptr && "TerrainFileService requires a command bus");

    initListeners(*m_cmd);

    auto terrainRoot = std::filesystem::path{m_configuredTerrainDir}.parent_path();
    if (terrainRoot.empty())
        terrainRoot = "terrains";

    TerrainLibraryReader reader;
    m_libraryFuture = reader.getFuture();

    auto payload = star::job::tasks::io::ReadPayload<TerrainLibraryReader>{
        .filePath = std::move(terrainRoot), .readFunction = std::move(reader), .kind = star::io::PathKind::directory};
    auto task = star::job::tasks::io::CreateReadTask(std::move(payload));
    star::command::file_io::ReadFromFile command{std::move(task)};
    m_cmd->submit(command);
}

void TerrainFileService::shutdown()
{
    if (m_cmd != nullptr)
        cleanupListeners(*m_cmd);
}

void TerrainFileService::initListeners(star::core::CommandBus &bus)
{
    m_onGetAll.init(bus);
    m_onGetCart.init(bus);
}

void TerrainFileService::cleanupListeners(star::core::CommandBus &bus)
{
    m_onGetAll.cleanup(bus);
    m_onGetCart.cleanup(bus);
}

void TerrainFileService::ensureLibraryBuilt()
{
    if (m_libraryBuilt)
        return;

    m_libraryBuilt = true;

    if (m_cmd == nullptr)
    {
        m_scanError = "TerrainFileService is not initialized";
        return;
    }

    std::optional<TerrainLibraryScanResult> scan;
    try
    {
        scan = m_libraryFuture.get();
    }
    catch (const std::exception &e)
    {
        m_scanError = e.what();
        return;
    }
    catch (...)
    {
        m_scanError = "unknown terrain library scan failure";
        return;
    }

    if (!scan.has_value() || !scan->has_value())
    {
        m_scanError = scan.has_value() ? scan->error() : std::string{"unknown terrain library scan failure"};
        return;
    }

    const auto &entries = scan->value();
    m_registrations.reserve(entries.size());
    m_libraries.reserve(entries.size());
    for (const auto &entry : entries)
        addEntry(entry.displayName, entry.terrainRootPath);
}

star::Handle TerrainFileService::addEntry(std::string displayName, std::filesystem::path terrainRootPath)
{
    const auto id = static_cast<std::uint32_t>(m_registrations.size());
    const star::Handle handle{.type = TerrainLibraryHandleType(), .id = id};

    TerrainLibraryEntryReader reader;
    auto future = reader.getFuture();

    auto payload = star::job::tasks::io::ReadPayload<TerrainLibraryEntryReader>{
        .filePath = terrainRootPath, .readFunction = std::move(reader), .kind = star::io::PathKind::directory};
    auto task = star::job::tasks::io::CreateReadTask(std::move(payload));
    star::command::file_io::ReadFromFile command{std::move(task)};
    m_cmd->submit(command);

    m_registrations.push_back(Registration{
        .handle = handle, .displayName = std::move(displayName), .terrainRootPath = std::move(terrainRootPath)});
    m_libraries.emplace(handle, EntryState{std::in_place_type<PendingTerrainLibrary>, std::move(future)});
    return handle;
}

TerrainFileService::Registration *TerrainFileService::registrationFor(star::Handle handle)
{
    if (handle.type != TerrainLibraryHandleType() || handle.id >= m_registrations.size())
        return nullptr;

    auto &registration = m_registrations[handle.id];
    if (!(registration.handle == handle))
        return nullptr;

    return &registration;
}

std::expected<TerrainLibrary, std::string> TerrainFileService::resolveEntry(star::Handle handle)
{
    auto *registration = registrationFor(handle);
    if (registration == nullptr)
        return std::unexpected(std::string{"Unknown terrain library handle"});

    if (registration->error.has_value())
        return std::unexpected(*registration->error);

    const auto it = m_libraries.find(handle);
    if (it == m_libraries.end())
        return std::unexpected(std::string{"Unknown terrain library handle"});

    if (auto *library = std::get_if<TerrainLibrary>(&it->second))
        return *library;

    auto &pending = std::get<PendingTerrainLibrary>(it->second);
    try
    {
        TerrainLibrary library = pending.get();
        it->second = std::move(library);
        return std::get<TerrainLibrary>(it->second);
    }
    catch (const std::exception &e)
    {
        registration->error = e.what();
        return std::unexpected(*registration->error);
    }
    catch (...)
    {
        registration->error = "Unknown terrain library read failure";
        return std::unexpected(*registration->error);
    }
}

void TerrainFileService::onGetAllTerrainLibraryRegistrations(command::GetAllTerrainLibraryRegistrations &cmd)
{
    try
    {
        ensureLibraryBuilt();

        command::GetAllTerrainLibraryRegistrationsReply reply;
        reply.scanError = m_scanError;
        reply.registrations.reserve(m_registrations.size());

        for (const auto &registration : m_registrations)
        {
            command::TerrainLibraryRegistration entry{.handle = registration.handle,
                                                      .displayName = registration.displayName,
                                                      .terrainRootPath = registration.terrainRootPath};

            auto resolved = resolveEntry(registration.handle);
            if (resolved)
                entry.terrainShapeFile = resolved->terrainShapeFile;
            else
                entry.error = resolved.error();

            reply.registrations.push_back(std::move(entry));
        }

        // Handles are assigned in scan order, but the map's iteration order is not used here; keep the reply
        // deterministic for callers that present or consume the list positionally.
        std::sort(reply.registrations.begin(), reply.registrations.end(),
                  [](const command::TerrainLibraryRegistration &lhs, const command::TerrainLibraryRegistration &rhs) {
                      if (lhs.displayName != rhs.displayName)
                          return lhs.displayName < rhs.displayName;
                      return lhs.terrainRootPath.filename().string() < rhs.terrainRootPath.filename().string();
                  });

        cmd.getReply().set(std::move(reply));
    }
    catch (const std::exception &e)
    {
        cmd.getReply().set(command::GetAllTerrainLibraryRegistrationsReply{.scanError = e.what()});
    }
    catch (...)
    {
        cmd.getReply().set(
            command::GetAllTerrainLibraryRegistrationsReply{.scanError = "Unknown terrain registrations failure"});
    }
}

void TerrainFileService::onGetLibraryCartForTerrain(command::GetLibraryCartForTerrain &cmd)
{
    try
    {
        ensureLibraryBuilt();

        if (m_scanError.has_value())
        {
            cmd.getReply().set(
                command::GetLibraryCartForTerrainResult{std::unexpected(CartReadException{*m_scanError})});
            return;
        }

        auto library = resolveEntry(cmd.handle);
        if (!library)
        {
            cmd.getReply().set(
                command::GetLibraryCartForTerrainResult{std::unexpected(CartReadException{library.error()})});
            return;
        }

        LibraryCartReader reader{library->terrainShapeFile, std::move(cmd.filters)};
        auto future = reader.getFuture();

        auto task = star::job::tasks::io::CreateReadTask(star::job::tasks::io::ReadPayload<LibraryCartReader>{
            .filePath = std::filesystem::path{library->terrainRootPath},
            .readFunction = std::move(reader),
            .kind = star::io::PathKind::directory});
        star::command::file_io::ReadFromFile readCommand{std::move(task)};
        m_cmd->submit(readCommand);

        cmd.getReply().set(command::GetLibraryCartForTerrainResult{PendingLibraryCart{std::move(future)}});
    }
    catch (const std::exception &e)
    {
        cmd.getReply().set(command::GetLibraryCartForTerrainResult{std::unexpected(CartReadException{e.what()})});
    }
    catch (...)
    {
        cmd.getReply().set(
            command::GetLibraryCartForTerrainResult{std::unexpected(CartReadException{"Unknown terrain cart error"})});
    }
}
} // namespace star::terrain::service

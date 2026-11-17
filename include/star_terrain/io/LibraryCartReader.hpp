#pragma once

#include "star_terrain/io/CartReadException.hpp"

#include <filesystem>
#include <future>
#include <memory>
#include <string>

namespace star::terrain
{
/// @brief Builds a LibraryCart for one terrain on the IO worker.
/// @details Seeded with the library's shape descriptor; the terrain root travels in the read payload so the reader
/// stays small enough for the task's inline storage. Call getFuture() before moving the reader into the task.
class LibraryCartReader
{
  public:
    using Result = LibraryCartReadResult;

    /// @param terrainShapeFile Shape descriptor filename relative to the root passed to operator().
    explicit LibraryCartReader(std::string terrainShapeFile);

    /// @brief Construct without a shape file; only valid until a task assigns one. The IO task machinery requires
    /// payloads to be default constructible.
    LibraryCartReader() = default;

    /// @brief Reads the terrain folder and stores the cart (or the failure) on the promise.
    /// @return 0 always.
    int operator()(const std::filesystem::path &terrainRoot);

    /// @brief Future resolving once the read task runs. Call before moving the reader.
    [[nodiscard]] std::future<Result> getFuture()
    {
        return m_result.get_future();
    }

  private:
    [[nodiscard]] Result read(const std::filesystem::path &terrainRoot) const;

    std::promise<Result> m_result;
    // Shared so the reader stays small enough for the IO task's inline payload storage.
    std::shared_ptr<const std::string> m_terrainShapeFile;
};
} // namespace star::terrain

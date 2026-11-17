#pragma once

#include "star_terrain/io/CartReadException.hpp"
#include "star_terrain/io/IFilter.hpp"

#include <filesystem>
#include <future>
#include <memory>
#include <string>
#include <vector>

namespace star::terrain
{

/// @brief Builds a LibraryCart for one terrain on the IO worker.
class LibraryCartReader
{
  public:
    using Result = LibraryCartReadResult;

    /// @param terrainShapeFile Shape descriptor filename relative to the root passed to operator().
    /// @param filters Predicates applied to the built cart; a chunk is kept only when every filter keeps it. No
    /// filters means every chunk is kept.
    LibraryCartReader(std::string terrainShapeFile, std::vector<std::unique_ptr<IFilter>> filters = {});

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
    struct Payload
    {
        std::string terrainShapeFile;
        std::vector<std::unique_ptr<IFilter>> filters;
    };

    [[nodiscard]] Result read(const std::filesystem::path &terrainRoot) const;

    std::unique_ptr<Payload> m_payload;
    std::promise<Result> m_result;
};
} // namespace star::terrain

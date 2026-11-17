#pragma once

#include "star_terrain/io/CartReadException.hpp"

#include <future>
#include <memory>

namespace star::terrain
{
/// @brief Handle to an in-flight cart read.
/// @details Copyable so it can travel through a ServiceReply (whose get() copies). Copies share one one-shot
/// future; only the first getLibraryCart() claims it.
class PendingLibraryCart
{
  public:
    using Result = LibraryCartReadResult;

    PendingLibraryCart() = default;
    explicit PendingLibraryCart(std::future<Result> future);
    PendingLibraryCart(const PendingLibraryCart &) = default;
    PendingLibraryCart &operator=(const PendingLibraryCart &) = default;
    PendingLibraryCart(PendingLibraryCart &&) noexcept = default;
    PendingLibraryCart &operator=(PendingLibraryCart &&) noexcept = default;
    ~PendingLibraryCart() = default;

    /// @brief Blocks until the IO worker satisfies the future. A second caller gets an error.
    [[nodiscard]] Result getLibraryCart();

    [[nodiscard]] bool valid() const noexcept;

  private:
    struct State;
    std::shared_ptr<State> m_state;
};
} // namespace star::terrain

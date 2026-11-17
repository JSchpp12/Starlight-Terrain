#include "star_terrain/io/PendingLibraryCart.hpp"

#include <atomic>
#include <exception>
#include <utility>

namespace star::terrain
{
struct PendingLibraryCart::State
{
    explicit State(std::future<Result> pending) : future(std::move(pending))
    {
    }

    std::future<Result> future;
    std::atomic_bool claimed{false};
};

PendingLibraryCart::PendingLibraryCart(std::future<Result> future) : m_state(std::make_shared<State>(std::move(future)))
{
}

bool PendingLibraryCart::valid() const noexcept
{
    return m_state != nullptr && m_state->future.valid();
}

PendingLibraryCart::Result PendingLibraryCart::getLibraryCart()
{
    if (m_state == nullptr)
        return std::unexpected(CartReadException{"Pending library cart is not initialized"});

    if (m_state->claimed.exchange(true))
        return std::unexpected(CartReadException{"Pending library cart was already consumed"});

    try
    {
        return m_state->future.get();
    }
    catch (const std::exception &e)
    {
        return std::unexpected(CartReadException{e.what()});
    }
    catch (...)
    {
        return std::unexpected(CartReadException{"unknown terrain cart read failure"});
    }
}
} // namespace star::terrain

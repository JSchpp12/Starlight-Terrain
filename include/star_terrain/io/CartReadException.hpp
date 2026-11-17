#pragma once

#include "star_terrain/io/LibraryCart.hpp"

#include <expected>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace star::terrain
{
/// @brief Raised when a terrain library cart cannot be read or built.
class CartReadException : public std::runtime_error
{
  public:
    explicit CartReadException(std::string message) : std::runtime_error(std::move(message))
    {
    }
};

/// @brief The value crossing the IO-thread boundary. LibraryCart is non-copyable and non-movable, so it
/// must travel behind a unique_ptr.
using LibraryCartReadResult = std::expected<std::unique_ptr<LibraryCart>, CartReadException>;
} // namespace star::terrain

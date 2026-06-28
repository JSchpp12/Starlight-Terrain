#pragma once

#include <string>

namespace star::terrain::rendering
{
enum class Type
{
    Flat,
    Real
};

constexpr std::string toString(Type type)
{
    switch (type)
    {
    case (Type::Flat):
        return "flat";
    case (Type::Real):
        return "real";
    }

    return "";
}
} // namespace star::terrain::rendering
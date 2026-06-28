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

constexpr Type fromString(std::string_view str)
{
    if (str == "flat")
        return Type::Flat;
    if (str == "real")
        return Type::Real;
    return Type::Flat;
}
} // namespace star::terrain::rendering
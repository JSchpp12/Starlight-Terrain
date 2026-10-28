#pragma once

#include <string>

namespace star::terrain::rendering
{
enum class ElevationType
{
    Flat,
    Real
};

constexpr std::string toString(ElevationType type)
{
    switch (type)
    {
    case (ElevationType::Flat):
        return "flat";
    case (ElevationType::Real):
        return "real";
    }

    return "";
}

constexpr ElevationType fromString(std::string_view str)
{
    if (str == "flat")
        return ElevationType::Flat;
    if (str == "real")
        return ElevationType::Real;
    return ElevationType::Flat;
}
} // namespace star::terrain::rendering
#pragma once

#include <string>
#include <string_view>

namespace star::terrain::rendering
{
enum class Representation
{
    Geometry,
    Height_Map
};

constexpr std::string representationToString(Representation representation)
{
    switch (representation)
    {
    case (Representation::Geometry):
        return "geometry";
    case (Representation::Height_Map):
        return "height_map";
    }

    return "";
}

constexpr Representation representationFromString(std::string_view str)
{
    if (str == "geometry")
        return Representation::Geometry;
    if (str == "height_map")
        return Representation::Height_Map;
    return Representation::Geometry;
}
} // namespace star::terrain::rendering

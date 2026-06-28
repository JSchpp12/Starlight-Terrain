#pragma once

#include <string>

namespace star::terrain
{
enum TerrainRenderingType
{
    Flat,
    Real
};

constexpr std::string toString(TerrainRenderingType type)
{
    switch (type)
    {
    case (TerrainRenderingType::Flat):
        return "flat";
    case (TerrainRenderingType::Real):
        return "real";
    }

    return "";
}
} // namespace star::terrain
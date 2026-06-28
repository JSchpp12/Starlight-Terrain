#pragma once

#include <glm/glm.hpp>

namespace star::terrain::util::distance{
    glm::dvec3 toECEF(const double &lat, const double &lon, const double &alt);

    glm::dmat3 getECEFToENUTransformation(const double &lat, const double &lon);

    double feetToMeters(const double &feet);

    double metersToFeet(const double &meters);

    double MilesToMeters(const double &miles); 
}
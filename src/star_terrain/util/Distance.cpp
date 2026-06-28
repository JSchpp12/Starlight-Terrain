#include "star_terrain/util/Distance.hpp"

namespace star::terrain::util::distance
{
glm::dvec3 toECEF(const double &lat, const double &lon, const double &alt)
{
    double a = 6378137.0;
    double e2 = 0.00669437999013;

    double latRad = glm::radians(lat);
    double lonRad = glm::radians(lon);

    double sinLatRad = std::sin(latRad);
    double e2sinLatSq = e2 * (sinLatRad * sinLatRad);

    double rn = a / std::sqrt((double)1.0f - e2sinLatSq);
    double R = (rn + alt) * std::cos(latRad);

    return glm::dvec3{R * std::cos(lonRad), R * std::sin(lonRad), (rn * ((double)1.0f - e2) + alt) * std::sin(latRad)};
}

glm::dmat3 getECEFToENUTransformation(const double &lat, const double &lon)
{
    const double phi = glm::radians(lat);
    const double lambda = glm::radians(lon);

    const double sinPhi = glm::sin(phi);
    const double cosPhi = glm::cos(phi);
    const double sinLambda = glm::sin(lambda);
    const double cosLambda = glm::cos(lambda);

    return glm::dmat3{
        -sinLambda,
        -cosLambda * sinPhi,
        cosLambda * cosPhi,
        cosLambda,
        -sinLambda * sinPhi,
        sinLambda * cosPhi,
        0.0,
        cosPhi,
        sinPhi,
    };
}

double feetToMeters(const double &feet)
{
    return feet * 0.3048;
}

double metersToFeet(const double &meters)
{
    return meters / 0.3048;
}

double MilesToMeters(const double &miles)
{
    const double MILES_TO_METERS = 1609.35;
    return miles * MILES_TO_METERS;
}
} // namespace star::terrain::util::distance
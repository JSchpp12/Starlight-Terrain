#include "star_terrain/rendering/BoundsDefinition.hpp"

#include "star_terrain/rendering/ShadowFrustumCalculator.hpp"

#include <starlight/core/Exceptions.hpp>
#include <starlight/virtual/StarCamera.hpp>

#include <cassert>
#include <utility>

namespace star::terrain::rendering
{
BoundsDefinition::Builder &BoundsDefinition::Builder::asBox(const glm::vec3 &min, const glm::vec3 &max)
{
    assert(min.x <= max.x && min.y <= max.y && min.z <= max.z && "Box fit target needs a valid AABB");

    for (int corner = 0; corner < 8; ++corner)
    {
        m_fitPoints[corner] = {((corner & 1) != 0) ? max.x : min.x, ((corner & 2) != 0) ? max.y : min.y,
                               ((corner & 4) != 0) ? max.z : min.z};
    }
    m_numFitPoints = 8;
    return *this;
}

BoundsDefinition::Builder &BoundsDefinition::Builder::asSphere(const glm::vec3 &center, float radius,
                                                               const glm::vec3 &lightDirection)
{
    assert(radius > 0.0f && "Sphere fit target needs a positive radius");

    // Quantize like the fit does, so a wobbling input radius can never change the texel size
    // between frames.
    radius = std::ceil(radius * 16.0f) / 16.0f;

    const glm::mat4 lightRot = ShadowFrustumCalculator::getLightRotation(lightDirection);
    const glm::vec3 centerInLight = glm::vec3(lightRot * glm::vec4(center, 1.0f));
    const glm::mat3 lightToWorld = glm::transpose(glm::mat3(lightRot)); // orthonormal rotation: transpose inverts

    for (int corner = 0; corner < 8; ++corner)
    {
        const glm::vec3 lightCorner =
            centerInLight + radius * glm::vec3(((corner & 1) != 0) ? 1.0f : -1.0f, ((corner & 2) != 0) ? 1.0f : -1.0f,
                                               ((corner & 4) != 0) ? 1.0f : -1.0f);
        m_fitPoints[corner] = lightToWorld * lightCorner;
    }
    m_numFitPoints = 8;
    return *this;
}

BoundsDefinition::Builder &BoundsDefinition::Builder::asCameraFrustumSphere(const star::StarCamera &worldCamera,
                                                                            const glm::vec3 &lightDirection)
{
    const ShadowFrustumCalculator calculator{worldCamera, lightDirection};
    const auto frustumInfo = calculator.getMainCameraFrustumInfo();
    return asSphere(frustumInfo.center, frustumInfo.viewSphereRadius, lightDirection);
}

BoundsDefinition BoundsDefinition::Builder::build() const
{
    if (m_numFitPoints == 0)
    {
        STAR_THROW("BoundsDefinition::Builder: no fit target was described before build");
    }

    return BoundsDefinition(m_fitPoints, m_numFitPoints);
}
} // namespace star::terrain::rendering
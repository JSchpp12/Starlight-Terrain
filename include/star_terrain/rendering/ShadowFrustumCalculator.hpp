#pragma once

#include "star_terrain/rendering/BoundsDefinition.hpp"

#include <array>

namespace star
{
class StarCamera;
class Light;
} // namespace star

namespace star::terrain::rendering
{
class ShadowFrustumCalculator
{
  public:
    struct FrustumCornerInfo
    {
        std::array<glm::vec3, 8> corners;
        glm::vec3 center{0.0f, 0.0f, 0.0f};
        float viewSphereRadius{0.0f};
    };
    struct LightProjectionParams
    {
        std::array<uint32_t, 3> resolution{2048, 2048, 0}; // z == 0: don't snap depth
        float casterPadding{0.0f};                         // extra depth past the fit target's light-facing face
    };

    ShadowFrustumCalculator(const star::StarCamera &worldCamera, const glm::vec3 &shadowLightDirection);
    FrustumCornerInfo getMainCameraFrustumInfo() const noexcept;
    FrustumCornerInfo getLightCameraFrustumInfo() const noexcept;
    void transformToLightSpace(FrustumCornerInfo &workingInfo) const noexcept;

    /// The fixed "light space" rotation (eye at origin, orientation only): world -> light. It
    /// depends only on the light direction -- never the camera -- so there is a stable texel grid
    /// to snap to. Also the basis light-space fit targets are built against (BoundsDefinition).
    static glm::mat4 getLightRotation(const glm::vec3 &lightDirection) noexcept;

    /// Fit the texel-snapped ortho shadow window to the given fit target. The xy window covers the
    /// target's points (snapped to the depth-map texel grid); the depth window runs from just
    /// sunward of them down past their light-facing face by params.casterPadding so the geometry
    /// below them still lands in the depth range.
    glm::mat4 getShadowLightProjectionForBounds(const BoundsDefinition &bounds,
                                                const LightProjectionParams &params) const noexcept;

  private:
    const star::StarCamera &m_worldCamera;
    const glm::vec3 &m_shadowLightDirection;
};
} // namespace star::terrain::rendering
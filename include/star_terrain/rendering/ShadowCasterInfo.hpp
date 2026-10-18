#pragma once

#include <glm/glm.hpp>

#include <array>

namespace star
{
class StarCamera;
class Light;
} // namespace star

namespace star::terrain::rendering
{
struct ShadowWindowBounds;

class ShadowCasterInfo
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
        float radius{0.0f};                                // <= 0: use the camera frustum sphere
        float casterPadding{0.0f};                         // extra depth on each side of the sphere
    };

    ShadowCasterInfo(const star::StarCamera &worldCamera, const glm::vec3 &shadowLightDirection);
    FrustumCornerInfo getMainCameraFrustumInfo() const noexcept;
    FrustumCornerInfo getLightCameraFrustumInfo() const noexcept;
    void transformToLightSpace(FrustumCornerInfo &workingInfo) const noexcept;
    glm::mat4 getShadowLightProjectionWithTexelSnapping(const LightProjectionParams &params) const noexcept;

    /// Fit the shadow window to the given world bounds instead of the main camera's view sphere.
    /// The xy window covers the region (snapped to the depth-map texel grid); the depth window runs
    /// from just sunward of the region down past its light-facing face by bounds.casterPadding so
    /// the geometry below it still lands in the depth range.
    glm::mat4 getShadowLightProjectionForWindowBounds(const LightProjectionParams &params,
                                                      const ShadowWindowBounds &windowBounds) const noexcept;

  private:
    const star::StarCamera &m_worldCamera;
    const glm::vec3 &m_shadowLightDirection;
};
} // namespace star::terrain::rendering
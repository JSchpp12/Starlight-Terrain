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
class ShadowCasterInfo
{
  public:
    struct FrustumCornerInfo
    {
        std::array<glm::vec3, 8> corners;
        glm::vec3 center{0.0f, 0.0f, 0.0f};
        float viewSphereRadius{0.0f};
    };

    ShadowCasterInfo(const star::StarCamera &worldCamera, const glm::vec3 &shadowLightDirection);
    FrustumCornerInfo getMainCameraFrustumInfo() const noexcept;
    void transformToLightSpace(FrustumCornerInfo &workingInfo) const noexcept;
    glm::mat4 getShadowLightProjection() const noexcept;

  private:
    const star::StarCamera &m_worldCamera;
    const glm::vec3 &m_shadowLightDirection;
};
} // namespace star::terrain::rendering
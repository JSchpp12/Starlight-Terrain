#include "star_terrain/rendering/ShadowCasterInfo.hpp"

#include <starlight/common/entities/Light.hpp>
#include <starlight/virtual/StarCamera.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <limits>

namespace star::terrain::rendering
{
ShadowCasterInfo::ShadowCasterInfo(const star::StarCamera &worldCamera, const glm::vec3 &shadowLightDirection)
    : m_worldCamera(worldCamera), m_shadowLightDirection(shadowLightDirection)
{
}

static std::array<glm::vec3, 8> GetNDFrustumCorners() noexcept
{
    return std::array<glm::vec3, 8>{
        glm::vec3(-1.0f, -1.0f, 0.0f), // near bottom-left
        glm::vec3(1.0f, -1.0f, 0.0f),  // near bottom-right
        glm::vec3(-1.0f, 1.0f, 0.0f),  // near top-left
        glm::vec3(1.0f, 1.0f, 0.0f),   // near top-right
        glm::vec3(-1.0f, -1.0f, 1.0f), // far bottom-left
        glm::vec3(1.0f, -1.0f, 1.0f),  // far bottom-right
        glm::vec3(-1.0f, 1.0f, 1.0f),  // far top-left
        glm::vec3(1.0f, 1.0f, 1.0f),   // far top-right
    };
}

static float GetViewFrustumSphereRadius(const std::array<glm::vec3, 8> &corners, const glm::vec3 &center) noexcept
{
    float radius{0.0f};
    for (const auto &corner : corners)
    {
        radius = glm::max(radius, glm::length(corner - center));
    }

    return radius;
}

ShadowCasterInfo::FrustumCornerInfo ShadowCasterInfo::getLightCameraFrustumInfo() const noexcept
{
    auto info = getMainCameraFrustumInfo();
    transformToLightSpace(info);
    return info;
}

ShadowCasterInfo::FrustumCornerInfo ShadowCasterInfo::getMainCameraFrustumInfo() const noexcept
{
    FrustumCornerInfo info{
        .corners = GetNDFrustumCorners(), .center = glm::vec3{0.0f, 0.0f, 0.0f}, .viewSphereRadius = 0.0f};

    const glm::mat4 inv = glm::inverse(m_worldCamera.getProjectionMatrix() * m_worldCamera.getViewMatrix());
    for (size_t i = 0; i < 8; ++i)
    {
        const glm::vec4 pt = inv * glm::vec4(info.corners[i], 1.0f);
        info.corners[i] = glm::vec3(pt) / pt.w;
        info.center += info.corners[i];
    }
    info.center /= 8.0f;

    info.viewSphereRadius = GetViewFrustumSphereRadius(info.corners, info.center);

    return info;
}

// glm::lookAt computes normalize(cross(forward, up)). If the light direction is (anti)parallel to the up vector that
// cross product is zero and the resulting matrix is NaN.
static glm::vec3 PickLightUpDirection(const glm::vec3 &lightDirection) noexcept
{
    const glm::vec3 a = glm::abs(lightDirection);
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    if (a.x <= a.y && a.x <= a.z)
        up = glm::vec3{1.0f, 0.0f, 0.0f};
    else if (a.y <= a.x && a.y <= a.z)
        up = glm::vec3{0.0f, 1.0f, 0.0f};
    else
        up = glm::vec3{0.0f, 0.0f, 1.0f};

    return up;
}

static glm::mat4 GetLightView(const glm::vec3 &frustumCenter, const glm::vec3 &lightDirection,
                              float sphereRadius) noexcept
{
    const glm::vec3 dir = glm::normalize(lightDirection);
    const auto up = PickLightUpDirection(lightDirection);
    const auto lightPosition = frustumCenter - (dir * sphereRadius);
    return glm::lookAt(lightPosition, frustumCenter, up);
}

static std::array<glm::vec3, 8> GetFrustumCornersLightSpace(const std::array<glm::vec3, 8> &worldCorners,
                                                            const glm::mat4 &lightView) noexcept
{
    std::array<glm::vec3, 8> lightSpaceCorners;
    for (size_t i = 0; i < worldCorners.size(); ++i)
    {
        const glm::vec4 transformed = lightView * glm::vec4(worldCorners[i], 1.0f);
        lightSpaceCorners[i] = glm::vec3(transformed);
    }
    return lightSpaceCorners;
}

struct LightSpaceAABB
{
    glm::vec3 min;
    glm::vec3 max;
};

static LightSpaceAABB GetLightSpaceAABB(const std::array<glm::vec3, 8> &lightSpaceCorners) noexcept
{
    glm::vec3 min{std::numeric_limits<float>::max()};
    glm::vec3 max{std::numeric_limits<float>::lowest()};
    for (const auto &corner : lightSpaceCorners)
    {
        min = glm::min(min, corner);
        max = glm::max(max, corner);
    }
    return LightSpaceAABB{min, max};
}

static glm::mat4 GetLightProjFromAABB(const LightSpaceAABB &aabb) noexcept
{
    constexpr float kDepthMargin = 1.0f;
    const float near = glm::max(0.0f, -aabb.max.z - kDepthMargin);
    const float far = -aabb.min.z + kDepthMargin;

    auto proj = glm::ortho(aabb.min.x, aabb.max.x, aabb.min.y, aabb.max.y, near, far);
    proj[1][1] *= -1;
    return proj;
}

static glm::mat4 GetLightViewProj(const std::array<glm::vec3, 8> &lightSpaceCorners,
                                  const glm::mat4 &lightView) noexcept
{
    const auto aabb = GetLightSpaceAABB(lightSpaceCorners);
    const auto lightProj = GetLightProjFromAABB(aabb);
    return lightProj * lightView;
}

void ShadowCasterInfo::transformToLightSpace(FrustumCornerInfo &workingInfo) const noexcept
{
    const auto lightView = GetLightView(workingInfo.center, m_shadowLightDirection, workingInfo.viewSphereRadius);
    workingInfo.corners = GetFrustumCornersLightSpace(workingInfo.corners, lightView);
}

glm::mat4 ShadowCasterInfo::getShadowLightProjection() const noexcept
{
    FrustumCornerInfo cornerInfo = getMainCameraFrustumInfo();
    const auto lightView = GetLightView(cornerInfo.center, m_shadowLightDirection, cornerInfo.viewSphereRadius);
    transformToLightSpace(cornerInfo);
    return GetLightViewProj(cornerInfo.corners, lightView);
}

} // namespace star::terrain::rendering
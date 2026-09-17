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

// glm::lookAt computes normalize(cross(forward, up)). Just pick the normal up direction for anything that is not
// directly down
static glm::vec3 PickLightUpDirection(const glm::vec3 &lightDirection, const glm::vec3 &cameraUpDir) noexcept
{
    const glm::vec3 forward = glm::normalize(lightDirection);
    const glm::vec3 up = glm::normalize(cameraUpDir);

    if (glm::abs(glm::dot(forward, up)) > 0.999f)
    {
        // Light points straight down/up: pick a horizontal axis so the shadow frustum's right/up axes line up with
        // world X/Z instead of getting swapped.
        return glm::vec3{0.0f, 0.0f, 1.0f};
    }

    return up;
}

static glm::mat4 GetLightView(const glm::vec3 &frustumCenter, const glm::vec3 &lightDirection,
                              const glm::vec3 cameraUpDir, float sphereRadius) noexcept
{
    const glm::vec3 forward = glm::normalize(lightDirection);
    glm::vec3 up = PickLightUpDirection(lightDirection, cameraUpDir);
    const glm::vec3 right = glm::normalize(glm::cross(forward, up));
    up = glm::cross(right, forward);

    const auto lightPosition = frustumCenter - (forward * sphereRadius);
    return glm::lookAt(lightPosition, frustumCenter, up);
}

// Rotation-only — NOT re-centered on frustumCenter, so a moving center actually moves in this space.
static glm::mat4 GetLightRotationOnly(const glm::vec3 &lightDirection, const glm::vec3 &cameraUpDir) noexcept
{
    const glm::vec3 forward = glm::normalize(lightDirection);
    glm::vec3 up = PickLightUpDirection(lightDirection, cameraUpDir);
    const glm::vec3 right = glm::normalize(glm::cross(forward, up));
    up = glm::cross(right, forward);
    return glm::lookAt(glm::vec3(0.0f), forward, up); // fixed eye/target, independent of frustumCenter
}

static std::array<glm::vec3, 8> ApplyTransformToCorners(const std::array<glm::vec3, 8> &worldCorners,
                                                        const glm::mat4 &transform) noexcept
{
    std::array<glm::vec3, 8> lightSpaceCorners;
    for (size_t i = 0; i < worldCorners.size(); ++i)
    {
        const glm::vec4 transformed = transform * glm::vec4(worldCorners[i], 1.0f);
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

static glm::mat4 GetLightProjFromSphere(const glm::vec3 &lightSpaceCenter, float radius, float near, float far) noexcept
{
    auto proj = glm::ortho(lightSpaceCenter.x - radius, lightSpaceCenter.x + radius, lightSpaceCenter.y - radius,
                           lightSpaceCenter.y + radius, near, far);
    proj[1][1] *= -1;
    return proj;
}

static glm::mat4 GetLightViewProj(const ShadowCasterInfo::FrustumCornerInfo &lightSpaceInfo, const glm::mat4 &lightView,
                                  const star::StarCamera &worldCamera) noexcept
{
    constexpr float kDepthMargin{1.0f};
    const auto aabb = GetLightSpaceAABB(lightSpaceInfo.corners);
    const float near = glm::max(0.0f, -aabb.max.z - kDepthMargin);
    const float far = -aabb.min.z + kDepthMargin;
    const auto lightProj = GetLightProjFromSphere(lightSpaceInfo.center, lightSpaceInfo.viewSphereRadius, near, far);

    return lightProj * lightView;
}

void ShadowCasterInfo::transformToLightSpace(FrustumCornerInfo &workingInfo) const noexcept
{
    const auto lightView = GetLightView(workingInfo.center, m_shadowLightDirection, m_worldCamera.getUpVector(),
                                        workingInfo.viewSphereRadius);
    workingInfo.corners = ApplyTransformToCorners(workingInfo.corners, lightView);
    workingInfo.center = glm::vec3(lightView * glm::vec4(workingInfo.center, 1.0));
}

static glm::vec3 SnapCameraPositiontoShadowTexel(const ShadowCasterInfo::FrustumCornerInfo &frustumInfo,
                                                 const std::array<uint32_t, 2> &shadowMapResolution,
                                                 const glm::vec3 &lightDir, const glm::vec3 &cameraUpDir) noexcept
{
    const auto lightView = GetLightRotationOnly(lightDir, cameraUpDir);
    const float orthoSize = frustumInfo.viewSphereRadius * 2.0f;
    const float texelSize = orthoSize / shadowMapResolution[0];

    auto lightSpaceCenter = lightView * glm::vec4(frustumInfo.center, 1.0);
    lightSpaceCenter.x = std::floor(lightSpaceCenter.x / texelSize) * texelSize;
    lightSpaceCenter.y = std::floor(lightSpaceCenter.y / texelSize) * texelSize;

    const glm::vec3 snappedCenter = glm::vec3(glm::inverse(lightView) * lightSpaceCenter);
    return snappedCenter;
}

glm::mat4 ShadowCasterInfo::getShadowLightProjectionWithTexelSnapping(
    const std::array<uint32_t, 2> &shadowMapResolution) const noexcept
{
    FrustumCornerInfo cornerInfo = getMainCameraFrustumInfo();
    // apply snapping
    cornerInfo.center = SnapCameraPositiontoShadowTexel(cornerInfo, shadowMapResolution, m_shadowLightDirection,
                                                        m_worldCamera.getUpVector());

    const auto lightView = GetLightView(cornerInfo.center, m_shadowLightDirection, m_worldCamera.getUpVector(),
                                        cornerInfo.viewSphereRadius);
    transformToLightSpace(cornerInfo);

    return GetLightViewProj(cornerInfo, lightView, m_worldCamera);
}

} // namespace star::terrain::rendering
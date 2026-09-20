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

// The light's orientation must depend ONLY on the light direction (never the camera), otherwise the shadow map
// rotates with the camera and there is no stable texel grid to snap to.
static glm::vec3 PickLightUpDirection(const glm::vec3 &forward) noexcept
{
    constexpr glm::vec3 kWorldUp{0.0f, 1.0f, 0.0f}; // change if your world is Z-up
    if (glm::abs(glm::dot(forward, kWorldUp)) > 0.999f)
    {
        return glm::vec3{0.0f, 0.0f, 1.0f};
    }
    return kWorldUp;
}

// Rotation only (eye at origin). This is the fixed "light space" used for snapping.
static glm::mat4 GetLightRotation(const glm::vec3 &lightDirection) noexcept
{
    const glm::vec3 forward = glm::normalize(lightDirection);
    return glm::lookAt(glm::vec3(0.0f), forward, PickLightUpDirection(forward));
}

// Light view whose eye sits `distanceBehind` back from `center` along the light direction.
static glm::mat4 GetLightView(const glm::vec3 &center, const glm::vec3 &lightDirection, float distanceBehind) noexcept
{
    const glm::vec3 forward = glm::normalize(lightDirection);
    const glm::vec3 eye = center - forward * distanceBehind;
    return glm::lookAt(eye, center, PickLightUpDirection(forward));
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

ShadowCasterInfo::FrustumCornerInfo ShadowCasterInfo::getLightCameraFrustumInfo() const noexcept
{
    auto info = getMainCameraFrustumInfo();
    transformToLightSpace(info);
    return info;
}

ShadowCasterInfo::FrustumCornerInfo ShadowCasterInfo::getMainCameraFrustumInfo() const noexcept
{
    FrustumCornerInfo info{.corners = GetNDFrustumCorners(), .center = glm::vec3{0.0f}, .viewSphereRadius = 0.0f};

    // NDC -> camera VIEW space. This depends only on the projection, so the shape (and radius) is identical no
    // matter where the camera is or where it is looking.
    const glm::mat4 invProj = glm::inverse(m_worldCamera.getProjectionMatrix());
    glm::vec3 viewCenter{0.0f};
    for (auto &corner : info.corners)
    {
        const glm::vec4 pt = invProj * glm::vec4(corner, 1.0f);
        corner = glm::vec3(pt) / pt.w;
        viewCenter += corner;
    }
    viewCenter /= 8.0f;

    // Quantize so tiny float differences can never change the texel size between frames.
    float radius = GetViewFrustumSphereRadius(info.corners, viewCenter);
    radius = std::ceil(radius * 16.0f) / 16.0f;

    // Now move to world space.
    const glm::mat4 invView = glm::inverse(m_worldCamera.getViewMatrix());
    ApplyTransformToCorners(info.corners, invView);
    info.center = glm::vec3(invView * glm::vec4(viewCenter, 1.0f));
    info.viewSphereRadius = radius;

    return info;
}

void ShadowCasterInfo::transformToLightSpace(FrustumCornerInfo &workingInfo) const noexcept
{
    const auto lightView = GetLightView(workingInfo.center, m_shadowLightDirection, workingInfo.viewSphereRadius);
    workingInfo.corners = ApplyTransformToCorners(workingInfo.corners, lightView);
    workingInfo.center = glm::vec3(lightView * glm::vec4(workingInfo.center, 1.0f));
}

glm::mat4 ShadowCasterInfo::getShadowLightProjectionWithTexelSnapping(
    const LightProjectionParams &params) const noexcept
{
    const FrustumCornerInfo info = getMainCameraFrustumInfo();

    // Quantize an overridden radius too, so it can never wobble between frames.
    const float radius = params.radius > 0.0f ? std::ceil(params.radius * 16.0f) / 16.0f : info.viewSphereRadius;
    const float depthRange = 2.0f * radius + 2.0f * params.casterPadding;

    const glm::mat4 lightRot = GetLightRotation(m_shadowLightDirection);
    glm::vec3 c = glm::vec3(lightRot * glm::vec4(info.center, 1.0f));

    const float extents[3] = {2.0f * radius, 2.0f * radius, depthRange};
    for (int i = 0; i < 3; ++i)
    {
        if (params.resolution[i] == 0)
            continue;
        const float step = extents[i] / static_cast<float>(params.resolution[i]);
        c[i] = std::floor(c[i] / step) * step;
    }

    const glm::mat4 lightView =
        glm::translate(glm::mat4(1.0f), glm::vec3(-c.x, -c.y, -c.z - radius - params.casterPadding)) * lightRot;

    auto proj = glm::ortho(-radius, radius, -radius, radius, 0.0f, depthRange);
    proj[1][1] *= -1;
    return proj * lightView;
}
} // namespace star::terrain::rendering
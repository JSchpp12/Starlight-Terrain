#include "star_terrain/rendering/ShadowFrustumCalculator.hpp"

#include <starlight/common/entities/Light.hpp>
#include <starlight/virtual/StarCamera.hpp>

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cassert>
#include <limits>

namespace star::terrain::rendering
{
ShadowFrustumCalculator::ShadowFrustumCalculator(const star::StarCamera &worldCamera,
                                                 const glm::vec3 &shadowLightDirection)
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

// Rotation only (eye at origin). This is the fixed "light space" used for snapping and for
// building light-space fit targets (BoundsDefinition::Builder::asSphere).
glm::mat4 ShadowFrustumCalculator::getLightRotation(const glm::vec3 &lightDirection) noexcept
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

ShadowFrustumCalculator::FrustumCornerInfo ShadowFrustumCalculator::getLightCameraFrustumInfo() const noexcept
{
    auto info = getMainCameraFrustumInfo();
    transformToLightSpace(info);
    return info;
}

ShadowFrustumCalculator::FrustumCornerInfo ShadowFrustumCalculator::getMainCameraFrustumInfo() const noexcept
{
    FrustumCornerInfo info{.corners = GetNDFrustumCorners(), .center = glm::vec3{0.0f}, .viewSphereRadius = 0.0f};

    // NDC -> camera VIEW space. This depends only on the projection, so the shape (and radius) is identical no
    // matter where the camera is or where it is looking.
    const glm::mat4 invProj = glm::inverse(m_worldCamera.getProjectionMatrix());
    for (auto &corner : info.corners)
    {
        const glm::vec4 pt = invProj * glm::vec4(corner, 1.0f);
        corner = glm::vec3(pt) / pt.w;
    }

    // Quantize so tiny float differences can never change the texel size between frames.
    float radius = GetViewFrustumSphereRadius(info.corners, glm::vec3{0.0});
    radius = std::ceil(radius * 16.0f) / 16.0f;

    // Now move to world space.
    const glm::mat4 invView = glm::inverse(m_worldCamera.getViewMatrix());
    ApplyTransformToCorners(info.corners, invView);
    info.center = m_worldCamera.getPosition();
    info.viewSphereRadius = radius;

    return info;
}

void ShadowFrustumCalculator::transformToLightSpace(FrustumCornerInfo &workingInfo) const noexcept
{
    const auto lightView = GetLightView(workingInfo.center, m_shadowLightDirection, workingInfo.viewSphereRadius);
    workingInfo.corners = ApplyTransformToCorners(workingInfo.corners, lightView);
    workingInfo.center = glm::vec3(lightView * glm::vec4(workingInfo.center, 1.0f));
}

glm::mat4 ShadowFrustumCalculator::getShadowLightProjectionForBounds(const BoundsDefinition &bounds,
                                                                     const LightProjectionParams &params) const noexcept
{
    const auto &fitPoints = bounds.getFitPoints();
    assert(!fitPoints.empty() && "Fit needs at least one point");

    const glm::mat4 lightRot = getLightRotation(m_shadowLightDirection);

    // Fit target bounds in light-rotation space. Light space +z faces the sun, so the largest z
    // point is the sunward face and the smallest z point is the light-facing face (the one toward
    // the ground).
    glm::vec3 mn(std::numeric_limits<float>::max());
    glm::vec3 mx(-std::numeric_limits<float>::max());
    for (const auto &worldPoint : fitPoints)
    {
        const glm::vec3 lightPoint = glm::vec3(lightRot * glm::vec4(worldPoint, 1.0f));
        mn = glm::min(mn, lightPoint);
        mx = glm::max(mx, lightPoint);
    }

    // xy window: cover the fit target with a small safety margin (snapping the center can only
    // shrink the covered area, never grow it), then quantize so the texel size can never wobble
    // between frames.
    float radius = 0.5f * glm::max(mx.x - mn.x, mx.y - mn.y) * 1.05f;
    radius = std::ceil(radius * 16.0f) / 16.0f;

    const float zExtent = mx.z - mn.z;

    // Depth window: start just sunward of the fit target and end past its light-facing face by the
    // caster padding so the geometry below it still lands in the depth range. The eye sits a little
    // above the sunward face instead of grazing it so near geometry is never clipped by the near
    // plane.
    const float nearMargin = glm::max(1.0f, 0.05f * zExtent);
    const float depthRange = zExtent + nearMargin + params.casterPadding;

    glm::vec3 c = (mn + mx) * 0.5f;
    const float snapExtents[3] = {2.0f * radius, 2.0f * radius, depthRange};
    for (int i = 0; i < 3; ++i)
    {
        if (params.resolution[i] == 0)
            continue;
        const float step = snapExtents[i] / static_cast<float>(params.resolution[i]);
        c[i] = std::floor(c[i] / step) * step;
    }

    const float eyeZ = c.z + 0.5f * zExtent + nearMargin;
    const glm::mat4 lightView = glm::translate(glm::mat4(1.0f), glm::vec3(-c.x, -c.y, -eyeZ)) * lightRot;

    auto proj = glm::ortho(-radius, radius, -radius, radius, 0.0f, depthRange);
    proj[1][1] *= -1;
    return proj * lightView;
}
} // namespace star::terrain::rendering
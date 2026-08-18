#include "star_terrain/rendering/ShadowCameraTransfer.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>
#include <starlight/virtual/StarCamera.hpp>

namespace star::terrain::rendering
{
ShadowCameraTransfer::ShadowCameraTransfer(glm::vec3 lightDirection, star::StarCamera mainRenderCamera)
    : m_lightDirection(std::move(lightDirection)), m_mainRenderCamera(std::move(mainRenderCamera))
{
}

std::unique_ptr<StarBuffers::Buffer> ShadowCameraTransfer::createStagingBuffer(vk::Device &device,
                                                                               VmaAllocator &allocator) const
{
    constexpr vk::DeviceSize size = sizeof(glm::mat4);

    return StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eExclusive)
                .setSize(size)
                .setUsage(vk::BufferUsageFlagBits::eTransferSrc),
            "ShadowCamera_TransferSRC")
        .setInstanceCount(1)
        .setInstanceSize(size)
        .buildUnique();
}

std::unique_ptr<StarBuffers::Buffer> ShadowCameraTransfer::createFinal(
    vk::Device &device, VmaAllocator &allocator, const std::vector<uint32_t> &transferQueueFamilyIndex) const
{
    constexpr vk::DeviceSize size = sizeof(glm::mat4);

    return StarBuffers::Buffer::Builder(allocator)
        .setAllocationCreateInfo(
            Allocator::AllocationBuilder()
                .setFlags(VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                .setUsage(VMA_MEMORY_USAGE_AUTO)
                .build(),
            vk::BufferCreateInfo()
                .setSharingMode(vk::SharingMode::eExclusive)
                .setSize(size)
                .setUsage(vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer),
            "ShadowCamera")
        .setInstanceCount(1)
        .setInstanceSize(size)
        .buildUnique();
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

static std::pair<std::array<glm::vec3, 8>, glm::vec3> GetFrustumCornersWorldSpace(const glm::mat4 &viewProj) noexcept
{
    auto corners = GetNDFrustumCorners();

    glm::vec3 center{0.0f, 0.0f, 0.0f};
    const glm::mat4 inv = glm::inverse(viewProj);
    for (size_t i = 0; i < 8; ++i)
    {
        const glm::vec4 pt = inv * glm::vec4(corners[i], 1.0f);
        corners[i] = glm::vec3(pt) / pt.w;
        center += corners[i];
    }
    center /= 8.0f;

    return std::make_pair(corners, center);
}

struct LightSpaceAABB
{
    glm::vec3 min;
    glm::vec3 max;
};

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

static glm::mat4 GetLightViewProj(const std::array<glm::vec3, 8> &worldCorners, const glm::vec3 &frustumCenter,
                                  const glm::vec3 &lightDirection, float sphereRadius) noexcept
{
    const auto lightView = GetLightView(frustumCenter, lightDirection, sphereRadius);
    const auto lightSpaceCorners = GetFrustumCornersLightSpace(worldCorners, lightView);
    const auto aabb = GetLightSpaceAABB(lightSpaceCorners);
    const auto lightProj = GetLightProjFromAABB(aabb);
    return lightProj * lightView;
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

ShadowCameraTransfer::ShadowCameraInfo ShadowCameraTransfer::getCameraInfo() const noexcept
{
    auto [corners, center] =
        GetFrustumCornersWorldSpace(m_mainRenderCamera.getProjectionMatrix() * m_mainRenderCamera.getViewMatrix());

    const float viewSphereRadius = GetViewFrustumSphereRadius(corners, center);
    const glm::mat4 lightProj = GetLightViewProj(corners, center, m_lightDirection, viewSphereRadius);

    return ShadowCameraInfo{.worldToLightViewProj = lightProj, .worldToShadowMapProj = glm::mat4()};
}

void ShadowCameraTransfer::writeDataToStageBuffer(StarBuffers::Buffer &buffer) const
{
    void *mapped = nullptr;
    buffer.map(&mapped);

    auto camInfo = getCameraInfo();
    buffer.writeToBuffer(&camInfo, mapped, sizeof(ShadowCameraInfo));

    buffer.flush();
    buffer.unmap();
}

} // namespace star::terrain::rendering

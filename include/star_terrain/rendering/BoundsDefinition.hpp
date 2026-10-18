#pragma once

#include <glm/glm.hpp>

#include <array>
#include <cstddef>
#include <span>

namespace star
{
class StarCamera;
} // namespace star

namespace star::terrain::rendering
{
class ShadowCasterInfo;

/// @brief The fit target for the shadow window: world-space points the texel-snapped ortho shadow
/// window must cover. Built through BoundsDefinition::Builder -- every way of describing the target
/// funnels into one validated point set, so the fit math never depends on how the target was
/// described.
class BoundsDefinition
{
  public:
    /// @brief An empty definition -- no points yet. The fit rejects it; getShadowWindowBounds fills one in.
    BoundsDefinition() = default;

    /// Fit targets are small: every construction path yields one box or sphere of exactly 8
    /// corners, so they live in a fixed array -- no heap allocation, including in the hot loop.
    static constexpr std::size_t kMaxFitPoints = 8;

    class Builder
    {
      public:
        Builder() = default;

        /// @brief Describe the target as the axis-aligned world box [min, max] (e.g. a fog volume's AABB).
        /// @param min Lower corner of the box.
        /// @param max Upper corner of the box.
        /// @return The builder.
        Builder &asBox(const glm::vec3 &min, const glm::vec3 &max);

        /// @brief Describe the target as the box bounding a `radius` sphere around `center` in
        /// light space. This target does not rotate with the camera, so the texel grid stays stable.
        /// @param center Sphere center (world space).
        /// @param radius Sphere radius.
        /// @param lightDirection Light direction the light space is built from.
        /// @return The builder.
        Builder &asSphere(const glm::vec3 &center, float radius, const glm::vec3 &lightDirection);

        /// @brief Describe the target as the sphere fit of the world camera's view frustum. The
        /// radius is quantized, so the texel size can never wobble between frames.
        /// @param worldCamera The camera whose view frustum defines the fit sphere.
        /// @param lightDirection Light direction the light space is built from.
        /// @return The builder.
        Builder &asCameraFrustumSphere(const star::StarCamera &worldCamera, const glm::vec3 &lightDirection);

        /// @brief Produce the definition; STAR_THROWs when no target was described.
        /// @return The definition.
        BoundsDefinition build() const;

      private:
        std::array<glm::vec3, BoundsDefinition::kMaxFitPoints> m_fitPoints{};
        std::size_t m_numFitPoints{0};
    };

    /// @brief World-space points the shadow window must cover.
    /// @return Span over the fit points.
    std::span<const glm::vec3> getFitPoints() const noexcept
    {
        return {m_fitPoints.data(), m_numFitPoints};
    }

    /// @brief Equality feeds the shadow window's change tracking (did the bounds change since last sync).
    /// @param other Other definition to compare against.
    /// @return True when both describe the same fit target.
    bool operator==(const BoundsDefinition &other) const = default;

  private:
    BoundsDefinition(std::array<glm::vec3, kMaxFitPoints> fitPoints, std::size_t numFitPoints)
        : m_fitPoints(fitPoints), m_numFitPoints(numFitPoints)
    {
    }

    std::array<glm::vec3, kMaxFitPoints> m_fitPoints{};
    std::size_t m_numFitPoints{0};
};
} // namespace star::terrain::rendering
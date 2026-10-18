#pragma once

#include <glm/glm.hpp>

#include <star_common/FrameTracker.hpp>

#include <cstdint>

namespace star::terrain::rendering
{
/// World-space bounds of the region the sun shadow window should cover. The shadow frustum is
/// fitted to these instead of the main camera's view sphere, so it keeps a stable, region-scale
/// texel grid no matter where the camera goes.
struct ShadowWindowBounds
{
    glm::vec3 min{};
    glm::vec3 max{};
    /// Extra depth past the region's light-facing face so geometry below it stays inside the
    /// shadow depth range and can still cast shadows into the region.
    float casterPadding{0.0f};
};

/// Source of the shadow window bounds plus per-frame-in-flight change tracking. Consumed by the
/// shadow camera controller, which refits the shadow projection whenever the bounds change.
class ShadowWindowBoundsProvider
{
  public:
    virtual ~ShadowWindowBoundsProvider() = default;

    /// False when no bounds are available; the caller falls back to the camera-frustum fit.
    virtual bool getShadowWindowBounds(ShadowWindowBounds &out) const = 0;

    /// True when the bounds changed since the last markShadowWindowBoundsSynced for the current
    /// frame in flight, mirroring the manager controller's willBeUpdatedThisFrame vocabulary.
    virtual bool willShadowWindowBoundsUpdate(const common::FrameTracker &frameTracker) const = 0;

    /// Record that the bounds were consumed for this frame in flight.
    virtual void markShadowWindowBoundsSynced(uint8_t frameInFlightIndex) = 0;
};
} // namespace star::terrain::rendering
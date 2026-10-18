#pragma once

#include "star_terrain/rendering/BoundsDefinition.hpp"

#include <star_common/FrameTracker.hpp>

#include <cstdint>

namespace star::terrain::rendering
{

/// @brief Source of the shadow window's fit target (a BoundsDefinition built over the tracked region's world AABB) plus
/// per-frame-in-flight change tracking.
class ShadowWindowBoundsProvider
{
  public:
    virtual ~ShadowWindowBoundsProvider() = default;

    /// @brief False when no bounds are available; the caller falls back to the camera-frustum fit.
    /// @param out
    /// @return
    virtual bool getShadowWindowBounds(BoundsDefinition &out) const = 0;

    /// @brief True when the bounds changed since the last markShadowWindowBoundsSynced for the current frame in flight
    /// @param frameTracker
    /// @return
    virtual bool willShadowWindowBoundsUpdate(const common::FrameTracker &frameTracker) const = 0;

    /// @brief Record that the bounds were consumed for this frame in flight.
    /// @param frameInFlightIndex
    virtual void markShadowWindowBoundsSynced(uint8_t frameInFlightIndex) = 0;
};
} // namespace star::terrain::rendering
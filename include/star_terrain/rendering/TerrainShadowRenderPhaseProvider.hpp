#pragma once

#include <starlight/common/entities/Light.hpp>
#include <starlight/core/renderer/FrameData.hpp>
#include <starlight/core/renderer/IRenderPhaseProvider.hpp>
#include <starlight/core/renderer/RenderPhaseConfig.hpp>
#include <starlight/core/renderer/RenderTargets.hpp>
#include <starlight/core/renderer/RenderingContext.hpp>
#include <starlight/object/StarObject.hpp>
#include <starlight/virtual/StarCamera.hpp>

#include <star_common/Handle.hpp>

#include <memory>
#include <vector>

namespace star::terrain
{
class TerrainShadowRenderPhase;

class TerrainShadowRenderPhaseProvider : public star::core::renderer::IRenderPhaseProvider
{
  public:
    TerrainShadowRenderPhaseProvider(star::core::device::DeviceContext &context,
                                     std::shared_ptr<std::vector<star::Light>> lights,
                                     std::shared_ptr<star::StarCamera> camera,
                                     std::vector<std::shared_ptr<star::StarObject>> objects, bool enableShadowCasting,
                                     star::Command_Buffer_Order_Index order);

    virtual ~TerrainShadowRenderPhaseProvider() = default;

    TerrainShadowRenderPhaseProvider(const TerrainShadowRenderPhaseProvider &) = delete;
    TerrainShadowRenderPhaseProvider &operator=(const TerrainShadowRenderPhaseProvider &) = delete;
    TerrainShadowRenderPhaseProvider(TerrainShadowRenderPhaseProvider &&) = default;
    TerrainShadowRenderPhaseProvider &operator=(TerrainShadowRenderPhaseProvider &&) = default;

    virtual std::unique_ptr<star::core::renderer::RenderPhase> build(
        star::core::device::DeviceContext &context, star::core::renderer::RenderPhaseRegistry &phases) override;

  private:
    star::core::renderer::RenderPhaseConfig m_config;
    std::vector<std::shared_ptr<star::StarObject>> m_objects;
    std::shared_ptr<star::core::renderer::FrameData> m_frameData;
    std::shared_ptr<std::vector<star::Light>> m_lights; // retained for the upcoming light-tracking controller
    bool m_enableShadowCasting = false;

    star::core::renderer::RenderTargets createRenderTargets(star::core::device::DeviceContext &ctx,
                                                            star::core::renderer::RenderingContext &renderingContext);
};
} // namespace star::terrain
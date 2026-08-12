#pragma once

#include <starlight/common/entities/Light.hpp>
#include <starlight/core/renderer/DefaultRenderPhaseProvider.hpp>
#include <starlight/core/renderer/RenderTargets.hpp>
#include <starlight/object/StarObject.hpp>
#include <starlight/virtual/StarCamera.hpp>

#include <star_common/Handle.hpp>

#include <memory>
#include <vector>

namespace star::terrain
{
class TerrainShadowRenderPhaseProvider : public star::core::renderer::DefaultRenderPhaseProvider
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

  protected:
    virtual star::core::renderer::RenderTargets createRenderTargets(
        core::device::DeviceContext &ctx, star::core::renderer::RenderingContext &renderingContext) override;

  private:
    bool m_enableShadowCasting = false;
};
} // namespace star::terrain

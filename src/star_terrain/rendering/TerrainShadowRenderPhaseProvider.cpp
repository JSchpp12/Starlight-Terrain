#include "star_terrain/rendering/TerrainShadowRenderPhaseProvider.hpp"

#include "star_terrain/rendering/ShadowCameraController.hpp"
#include "star_terrain/rendering/TerrainShadowRenderPhase.hpp"

#include <cassert>
#include <functional>
#include <memory>
#include <star_common/EventBus.hpp>
#include <star_common/FrameTracker.hpp>
#include <star_common/Handle.hpp>
#include <star_common/HandleTypeRegistry.hpp>
#include <starlight/command/command_order/DeclarePass.hpp>
#include <starlight/common/controllers/ManagerController_RenderResource_GlobalInfo.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/core/device/DeviceContext.hpp>
#include <starlight/core/device/managers/Semaphore.hpp>
#include <starlight/core/device/system/event/ManagerRequest.hpp>
#include <starlight/core/helper/command_buffer/CommandBufferHelpers.hpp>
#include <starlight/core/helper/queue/QueueHelpers.hpp>
#include <starlight/core/renderer/DescriptorRecipe.hpp>
#include <starlight/core/renderer/FrameData.hpp>
#include <starlight/event/DescriptorPoolReady.hpp>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace star::terrain
{

static std::vector<star::Handle> CreateSemaphores(star::common::EventBus &evtBus,
                                                  const star::common::FrameTracker &ft) noexcept
{
    const size_t num = static_cast<size_t>(ft.getSetup().getNumFramesInFlight());

    auto handles = std::vector<star::Handle>(num);
    for (size_t i{0}; i < handles.size(); i++)
    {
        void *r = nullptr;
        evtBus.emit(star::core::device::system::event::ManagerRequest(
            star::common::HandleTypeRegistry::instance().getTypeGuaranteedExist(
                star::core::device::manager::GetSemaphoreEventTypeName),
            star::core::device::manager::SemaphoreRequest{true}, handles[i], &r));

        if (r == nullptr)
        {
            STAR_THROW("Unable to create new semaphore");
        }
    }

    return handles;
}

static std::shared_ptr<star::core::renderer::FrameData> CreateShadowFrameData(
    star::core::device::DeviceContext &context, std::shared_ptr<std::vector<star::Light>> lights)
{
    auto cameraController = std::make_shared<star::terrain::rendering::ShadowCameraController>(
        context.frameTracker().getSetup().getNumFramesInFlight(), lights, 0);
    auto fd = std::make_shared<star::core::renderer::FrameData>();
    fd->add(std::move(cameraController), star::core::renderer::roleHandle(star::core::renderer::frame_roles::Camera));
    return fd;
}

static void RegisterWithCommandOrder(const star::core::CommandBus &cmdBus, star::common::EventBus &evtBus,
                                     star::core::device::manager::Queue &qm, star::Handle commandBuffer)
{
    auto *queue = star::core::helper::GetEngineDefaultQueue(evtBus, qm, star::Queue_Type::Tgraphics);
    assert(queue != nullptr && "Failed to acquire default engine queue");

    cmdBus.submit(star::command_order::DeclarePass{std::move(commandBuffer), queue->getParentQueueFamilyIndex()});
}

static std::vector<star::StarRenderGroup> CreateRenderingGroups(star::core::device::DeviceContext &context,
                                                                std::vector<std::shared_ptr<star::StarObject>> objects)
{
    auto renderingGroups = std::vector<star::StarRenderGroup>();

    for (size_t i = 0; i < objects.size(); i++)
    {
        star::StarRenderGroup *match = nullptr;

        for (size_t j = 0; j < renderingGroups.size(); j++)
        {
            if (renderingGroups[j].isObjectCompatible(*objects[i]))
            {
                match = &renderingGroups[j];
                break;
            }
        }

        if (match != nullptr)
        {
            match->addObject(objects[i]);
        }
        else
        {
            renderingGroups.emplace_back(context, objects[i]);
        }
    }

    return renderingGroups;
}

TerrainShadowRenderPhaseProvider::TerrainShadowRenderPhaseProvider(
    star::core::device::DeviceContext &context, std::shared_ptr<std::vector<star::Light>> lights,
    std::vector<std::shared_ptr<star::StarObject>> objects, bool enableShadowCasting,
    star::Command_Buffer_Order_Index order)
    : m_objects(std::move(objects)), m_frameData(CreateShadowFrameData(context, lights)), m_lights(std::move(lights)),
      m_enableShadowCasting(enableShadowCasting)
{
    m_config.order = star::Command_Buffer_Order::before_render_pass;
    m_config.orderIndex = order;
    m_config.waitStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
}

star::core::renderer::RenderTargets TerrainShadowRenderPhaseProvider::createRenderTargets(
    star::core::device::DeviceContext &ctx, star::core::renderer::RenderingContext &renderingContext)
{
    auto depthTextures = star::core::renderer::RenderTargets::createDefaultDepthAttachments(
        ctx, static_cast<size_t>(ctx.frameTracker().getSetup().getNumFramesInFlight()),
        renderingContext.targetResolution.width, renderingContext.targetResolution.height);

    auto *graphicsQueue = star::core::helper::GetEngineDefaultQueue(
        ctx.getEventBus(), ctx.getGraphicsManagers().queueManager, star::Queue_Type::Tpresent);
    assert(graphicsQueue != nullptr);

    auto depthHandles = star::core::renderer::RenderTargets::registerTextures(ctx, renderingContext, depthTextures);

    for (const auto &th : depthHandles)
    {
        const auto &tx = ctx.getGraphicsManagers().imageManager.get(th)->texture;

        auto oneTimeSetup = star::core::helper::BeginSingleTimeCommands(
            ctx.getDevice(), ctx.getEventBus(), ctx.getManagerCommandBuffer().m_manager, star::Queue_Type::Tgraphics);

        vk::ImageMemoryBarrier2 barrier[1]{vk::ImageMemoryBarrier2()
                                               .setOldLayout(vk::ImageLayout::eUndefined)
                                               .setNewLayout(vk::ImageLayout::eDepthAttachmentOptimal)
                                               .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                                               .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
                                               .setImage(tx.getVulkanImage())
                                               .setSrcAccessMask(vk::AccessFlagBits2::eNone)
                                               .setSrcStageMask(vk::PipelineStageFlagBits2::eNone)
                                               .setDstAccessMask(vk::AccessFlagBits2::eDepthStencilAttachmentRead |
                                                                 vk::AccessFlagBits2::eDepthStencilAttachmentWrite)
                                               .setDstStageMask(vk::PipelineStageFlagBits2::eEarlyFragmentTests)
                                               .setSubresourceRange(vk::ImageSubresourceRange()
                                                                        .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                                                        .setBaseMipLevel(0)
                                                                        .setLevelCount(vk::RemainingMipLevels)
                                                                        .setBaseArrayLayer(0)
                                                                        .setLayerCount(vk::RemainingArrayLayers))};

        oneTimeSetup.buffer().pipelineBarrier2(vk::DependencyInfo().setImageMemoryBarriers(barrier));
        star::core::helper::EndSingleTimeCommands(*graphicsQueue, std::move(oneTimeSetup));
    }

    return star::core::renderer::RenderTargets({}, std::nullopt, std::move(depthHandles),
                                               depthTextures.front().getBaseFormat());
}

std::unique_ptr<star::core::renderer::RenderPhase> TerrainShadowRenderPhaseProvider::build(
    star::core::device::DeviceContext &c, star::core::renderer::RenderPhaseRegistry & /*phases*/)
{
    auto phase = std::make_unique<TerrainShadowRenderPhase>(m_enableShadowCasting);

    phase->m_objects = std::move(m_objects);
    phase->m_frameData = m_frameData;
    phase->setDataRolesOwned(star::core::renderer::roleHandle(star::core::renderer::frame_roles::Camera));

    phase->m_renderGroups = CreateRenderingGroups(c, phase->m_objects);

    auto request = star::core::device::manager::ManagerCommandBuffer::Request{
        .recordBufferCallback = std::bind(&TerrainShadowRenderPhase::recordCommandBuffer, phase.get(),
                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
        .order = m_config.order,
        .orderIndex = m_config.orderIndex,
        .type = m_config.queueType,
        .waitStage = m_config.waitStage,
        .willBeSubmittedEachFrame = m_config.willBeSubmittedEachFrame,
        .recordOnce = m_config.recordOnce,
        .overrideBufferSubmissionCallback = phase->getSubmissionOverride(),
    };
    phase->m_commandBuffer =
        c.getManagerCommandBuffer().submit(std::move(request), c.frameTracker().getCurrent().getGlobalFrameCounter());
    RegisterWithCommandOrder(c.getCmdBus(), c.getEventBus(), c.getGraphicsManagers().queueManager,
                             phase->m_commandBuffer);

    phase->m_frameData->prepRender(c, c.frameTracker().getSetup().getNumFramesInFlight());

    phase->m_renderingContext.targetResolution = vk::Extent2D().setHeight(2048).setWidth(2048);
    phase->m_renderTargets = createRenderTargets(c, phase->m_renderingContext);

    for (auto &group : phase->m_renderGroups)
        group.prepRender(c);

    const auto global = star::core::renderer::shaderInfoHandle("Global");
    star::core::renderer::DescriptorRecipe::Builder(c.getEventBus(), c,
                                                    star::event::DescriptorPoolReady::GetUniqueTypeName())
        .setShaderInfoOut(global, &phase->m_globalShaderInfo)
        .addBinding(global, 0, phase->m_frameData,
                    star::core::renderer::roleHandle(star::core::renderer::frame_roles::Camera), 0,
                    vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eAll)
        .setRenderGroups(global, &phase->m_renderGroups, phase->getRenderTargetInfo(), phase->m_commandBuffer)
        .build();

    // Shadow-specific tail: timeline semaphores + cmd-bus/device handles used by
    // the phase's self-trigger (frameUpdate) and submission override.
    phase->m_cmdBus = &c.getCmdBus();
    phase->m_device = c.getDevice().getVulkanDevice();
    phase->m_timelineSemaphores = CreateSemaphores(c.getEventBus(), c.frameTracker());

    return phase;
}
} // namespace star::terrain
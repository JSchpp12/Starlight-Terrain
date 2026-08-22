#include "star_terrain/rendering/TerrainShadowRenderPhaseProvider.hpp"

#include "star_terrain/rendering/DataRoles.hpp"
#include "star_terrain/rendering/ShadowCameraController.hpp"
#include "star_terrain/rendering/TerrainShadowRenderPhase.hpp"

#include <starlight/common/controllers/ManagerController_RenderResource_GlobalInfo.hpp>
#include <starlight/common/controllers/ManagerController_RenderResource_InstanceModelInfo.hpp>
#include <starlight/core/Exceptions.hpp>
#include <starlight/core/device/DeviceContext.hpp>
#include <starlight/core/helper/command_buffer/CommandBufferHelpers.hpp>
#include <starlight/core/helper/queue/QueueHelpers.hpp>
#include <starlight/core/renderer/DescriptorRecipe.hpp>
#include <starlight/core/renderer/FrameData.hpp>
#include <starlight/core/renderer/RenderPhaseHelpers.hpp>
#include <starlight/event/DescriptorPoolReady.hpp>

#include <star_common/EventBus.hpp>
#include <star_common/FrameTracker.hpp>
#include <star_common/Handle.hpp>

#include <cassert>
#include <functional>
#include <memory>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace star::terrain
{

static const star::ManagerController::RenderResource::GlobalInfo *GetMainGlobalInfo(
    const star::core::renderer::RenderPhaseRegistry &phases, const star::Handle &mainReg) noexcept
{
    const auto *mainTerrain = phases.getPhase(mainReg);
    assert(mainTerrain != nullptr &&
           "Main terrain render phase provider needs to be supplied to the manager before the terrain shadow render");
    const auto *mainController = mainTerrain->getFrameData()->getController(
        star::core::renderer::roleHandle(star::core::renderer::frame_roles::Camera));
    assert(mainController != nullptr);

    const auto *cameraController =
        dynamic_cast<const star::ManagerController::RenderResource::GlobalInfo *>(mainController);
    assert(cameraController != nullptr &&
           "The main renderer is expected to have a GlobalInfo type for its main controller. However, a "
           "different unsupported type was encountered");

    return cameraController;
}

static std::shared_ptr<star::core::renderer::FrameData> CreateShadowFrameData(
    star::core::device::DeviceContext &context, std::shared_ptr<std::vector<star::Light>> lights,
    const star::ManagerController::RenderResource::GlobalInfo *mainCamController,
    const star::ManagerController::RenderResource::InstanceModelInfo *instanceModelInfoController) noexcept
{
    auto cameraController = std::make_shared<star::terrain::rendering::ShadowCameraController>(
        context.frameTracker().getSetup().getNumFramesInFlight(), lights, 0, mainCamController,
        instanceModelInfoController);
    auto fd = std::make_shared<star::core::renderer::FrameData>();
    fd->add(std::move(cameraController),
            star::core::renderer::roleHandle(star::terrain::rendering::data_roles::ShadowLightProjections));
    return fd;
}

TerrainShadowRenderPhaseProvider::TerrainShadowRenderPhaseProvider(
    star::core::device::DeviceContext &context, std::shared_ptr<std::vector<star::Light>> lights,
    std::vector<std::shared_ptr<star::StarObject>> objects, star::Handle mainTerrainRenderPhaseRegistration,
    bool enableShadowCasting, star::Command_Buffer_Order_Index order)
    : m_objects(std::move(objects)), m_lights(std::move(lights)),
      m_mainTerrainRenderRegistration(std::move(mainTerrainRenderPhaseRegistration)),
      m_enableShadowCasting(enableShadowCasting)
{
    m_config.order = star::Command_Buffer_Order::before_render_pass;
    m_config.orderIndex = order;
    m_config.waitStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
}

static vk::Format selectFormat(core::device::DeviceContext &device, const std::vector<vk::Format> &candidates,
                               vk::FormatFeatureFlags features)
{
    vk::Format selected = vk::Format();
    if (!device.getDevice().findSupportedFormat(candidates, vk::ImageTiling::eOptimal, features, selected))
        STAR_THROW("RenderTargets: failed to find a supported format for the requested features");
    return selected;
}

static std::vector<StarTextures::Texture> CreateShadowDepthTextures(core::device::DeviceContext &context,
                                                                    const size_t numToCreate, int width, int height)
{
    const auto &props = context.getDevice().getPhysicalDevice().getProperties();

    vk::Format depthFormat = vk::Format::eUndefined;
    std::vector<StarTextures::Texture> depthTextures;
    depthTextures.reserve(numToCreate);
    {
        depthFormat =
            selectFormat(context, {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
                         vk::FormatFeatureFlagBits::eDepthStencilAttachment | vk::FormatFeatureFlagBits::eSampledImage);

        auto builder =
            star::StarTextures::Texture::Builder(context.getDevice().getVulkanDevice(),
                                                 context.getDevice().getAllocator().get())
                .setCreateInfo(
                    Allocator::AllocationBuilder()
                        .setFlags(VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT)
                        .setUsage(VMA_MEMORY_USAGE_GPU_ONLY)
                        .build(),
                    vk::ImageCreateInfo()
                        .setExtent(vk::Extent3D().setWidth(width).setHeight(height).setDepth(1))
                        .setArrayLayers(1)
                        .setSharingMode(vk::SharingMode::eExclusive)
                        .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled)
                        .setImageType(vk::ImageType::e2D)
                        .setMipLevels(1)
                        .setTiling(vk::ImageTiling::eOptimal)
                        .setInitialLayout(vk::ImageLayout::eUndefined)
                        .setSamples(vk::SampleCountFlagBits::e1),
                    "OffscreenRenderToImagesDepth")
                .setBaseFormat(depthFormat)
                .addViewInfo(vk::ImageViewCreateInfo()
                                 .setViewType(vk::ImageViewType::e2D)
                                 .setFormat(depthFormat)
                                 .setSubresourceRange(vk::ImageSubresourceRange()
                                                          .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                                          .setBaseArrayLayer(0)
                                                          .setLayerCount(1)
                                                          .setBaseMipLevel(0)
                                                          .setLevelCount(1)))
                .setSamplerInfo(vk::SamplerCreateInfo()
                                    .setAnisotropyEnable(true)
                                    .setMaxAnisotropy(star::StarTextures::Texture::SelectAnisotropyLevel(props))
                                    .setMagFilter(vk::Filter::eLinear)
                                    .setMinFilter(vk::Filter::eLinear)
                                    .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
                                    .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
                                    .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
                                    .setBorderColor(vk::BorderColor::eIntOpaqueWhite)
                                    .setUnnormalizedCoordinates(vk::False)
                                    .setCompareEnable(vk::True)
                                    .setCompareOp(vk::CompareOp::eLessOrEqual)
                                    .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                                    .setMipLodBias(0.0f)
                                    .setMinLod(0.0f)
                                    .setMaxLod(0.0f));

        for (uint8_t i = 0; i < numToCreate; i++)
        {
            depthTextures.push_back(builder.build());
        }
    }

    return depthTextures;
}

star::core::renderer::RenderTargets TerrainShadowRenderPhaseProvider::createRenderTargets(
    star::core::device::DeviceContext &ctx, star::core::renderer::RenderingContext &renderingContext)
{
    auto depthTextures =
        CreateShadowDepthTextures(ctx, static_cast<size_t>(ctx.frameTracker().getSetup().getNumFramesInFlight()),
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
                                               .setNewLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
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
    star::core::device::DeviceContext &c, star::core::renderer::RenderPhaseRegistry &phases)
{
    const auto shadowLightProjDataRole =
        star::core::renderer::roleHandle(star::terrain::rendering::data_roles::ShadowLightProjections);
    auto phase = std::make_unique<TerrainShadowRenderPhase>(c.getCmdBus(), c.getDevice().getVulkanDevice(),
                                                            m_enableShadowCasting);
    phase->m_objects = std::move(m_objects);

    {
        assert(!phase->m_objects.empty() && "Shadow-cast terrain must always be provided");
        const auto *mainCamera = GetMainGlobalInfo(phases, m_mainTerrainRenderRegistration);
        auto *instanceModel = &phase->m_objects.front()->getInstanceModelController();
        phase->m_frameData = CreateShadowFrameData(c, m_lights, mainCamera, instanceModel);
    }

    phase->setDataRolesOwned(shadowLightProjDataRole);
    phase->m_renderGroups = star::core::renderer::CreateRenderingGroups(c, phase->m_objects);

    phase->graphicsQueueFamilyIndex =
        star::core::helper::GetEngineDefaultQueue(c.getEventBus(), c.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Tgraphics)
            ->getParentQueueFamilyIndex();
    phase->computeQueueFamilyIndex =
        star::core::helper::GetEngineDefaultQueue(c.getEventBus(), c.getGraphicsManagers().queueManager,
                                                  star::Queue_Type::Tcompute)
            ->getParentQueueFamilyIndex();
    phase->m_shadowDepthReleasePolicy = star::terrain::rendering::makeShadowDepthReleasePolicy(
        phase->graphicsQueueFamilyIndex, phase->computeQueueFamilyIndex);
    phase->m_firstFramePassCounter = static_cast<uint32_t>(c.frameTracker().getSetup().getNumFramesInFlight());
    phase->m_shadowDepthReacquirePolicy = star::terrain::rendering::makeShadowDepthReacquirePolicy(
        phase->graphicsQueueFamilyIndex, phase->computeQueueFamilyIndex);

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
    star::core::renderer::RegisterWithCommandOrder(c.getCmdBus(), c.getEventBus(), c.getGraphicsManagers().queueManager,
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
        .addBinding(global, 0, phase->m_frameData, shadowLightProjDataRole, 0, vk::DescriptorType::eUniformBuffer,
                    vk::ShaderStageFlagBits::eAll)
        .setRenderGroups(global, &phase->m_renderGroups, phase->getRenderTargetInfo(), phase->m_commandBuffer)
        .build();
    // Shadow-specific tail: timeline semaphores used by the phase's self-trigger (frameUpdate).
    phase->m_timelineSemaphores = star::core::renderer::CreateSemaphores(c.getEventBus(), c.frameTracker());

    return phase;
}
} // namespace star::terrain

#include "vulkan_backend/VulkanRenderer.h"

#include <algorithm>
#include <stdexcept>
#include <renderer/Vertex.h>

#include "vulkan_backend/command_buffer_sync.h"
#include "core/constants.h"
#include "vulkan_backend/Rendering.h"
#include "vulkan_backend/swapchain_management.h"
#include "vulkan_backend/vulkan_init.h"


VulkanRenderer::~VulkanRenderer()
{
    waitIdle(); // 先停 GPU

    destroyFrameResources();
    destroySwapchain();        // 含 framebuffers/renderPass/pipeline/commandBuffers/commandPool
    // 先销毁依赖 VkDevice 的高层管理器（它们内部会销毁 Buffer/Image/Sampler 等）
    descriptorSetManager_.reset();
    resourceManager_.reset();
     
    
    destroyDeviceAndSurface(); // VkDevice/VkSurfaceKHR
    destroyInstance();         // VkInstance
}

void VulkanRenderer::initialize(void* windowHandle, int width, int height)
{
    (void)width;
    (void)height; // 目前 swapchain 尺寸从 surface capabilities 推导

    createInstanceAndSurface(windowHandle);
    createDeviceAndQueues();
    createResourceManagers();

    createSwapchain();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createFrameResources();

    swapchainOutOfDate_ = false;
    currentFrameIndex_  = 0;
    currentImageIndex_  = 0;
}

void VulkanRenderer::resize(int width, int height)
{
    if (width == 0 || height == 0)
        return; // 最小化等情况，不重建

    waitIdle();

    destroyFrameResources();
    destroySwapchain();

    createSwapchain();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createFrameResources();

    swapchainOutOfDate_ = false;
}


bool VulkanRenderer::beginFrame(const FrameContext& ctx)
{
    if (framesInFlight_.empty())
        return false;

    FrameResources& frame = framesInFlight_[currentFrameIndex_];

    // 1. 等当前帧 fence
    if (frame.inFlightFence != VK_NULL_HANDLE)
    {
        VK_CHECK(vkWaitForFences(device_->device(), 1, &frame.inFlightFence, VK_TRUE, UINT64_MAX));
        VK_CHECK(vkResetFences(device_->device(), 1, &frame.inFlightFence));
    }

    // 2. Acquire image
    VkResult acquireRes = vkAcquireNextImageKHR(
                                                device_->device(),
                                                swapchain_.swapchain,
                                                UINT64_MAX,
                                                frame.imageAvailableSemaphore,
                                                VK_NULL_HANDLE,
                                                &currentImageIndex_);

    if (acquireRes == VK_ERROR_OUT_OF_DATE_KHR)
    {
        swapchainOutOfDate_ = true;
        return false; // 让上层驱动 resize()
    }
    else if (acquireRes == VK_SUBOPTIMAL_KHR)
    {
        swapchainOutOfDate_ = true; // 当前帧仍可渲染
    }
    else if (acquireRes != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to acquire swapchain image");
    }

    // // 3. 保护每个 image 只被一个 in-flight frame 使用
    // if (!imagesInFlight_.empty())
    // {
    //     if (imagesInFlight_[currentImageIndex_] != VK_NULL_HANDLE)
    //     {
    //         VK_CHECK(vkWaitForFences(device_->device(),
    //                      1,
    //                      &imagesInFlight_[currentImageIndex_],
    //                      VK_TRUE,
    //                      UINT64_MAX));
    //     }
    //     imagesInFlight_[currentImageIndex_] = frame.inFlightFence;
    // }

    // TODO: 利用 ctx（时间、帧号等）更新 UBO / push constants
    (void)ctx;

    // 4. 录制本帧命令
    recordCommandBuffer(frame, currentImageIndex_);

    return true;
}


void VulkanRenderer::renderFrame()
{
    if (framesInFlight_.empty())
        return;

    FrameResources& frame = framesInFlight_[currentFrameIndex_];

    if (frame.commandBuffer == VK_NULL_HANDLE)
        throw std::runtime_error("Frame command buffer is null in renderFrame");

    const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = &frame.imageAvailableSemaphore;
    submitInfo.pWaitDstStageMask    = &waitStage;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &frame.commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = &frame.renderFinishedSemaphore;

    VK_CHECK(vkQueueSubmit(device_->graphicsQueue().handle, 1, &submitInfo, frame.inFlightFence));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = &frame.renderFinishedSemaphore;
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &swapchain_.swapchain;
    presentInfo.pImageIndices      = &currentImageIndex_;

    VkResult presentRes = vkQueuePresentKHR(device_->presentQueue().handle, &presentInfo);
    if (presentRes == VK_ERROR_OUT_OF_DATE_KHR || presentRes == VK_SUBOPTIMAL_KHR)
    {
        swapchainOutOfDate_ = true;
    }
    else if (presentRes != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to present swapchain image");
    }

    currentFrameIndex_ = (currentFrameIndex_ + 1) % static_cast<uint32_t>(framesInFlight_.size());
}

void VulkanRenderer::waitIdle()
{
    if (device_ && device_->device() != VK_NULL_HANDLE)
        vkDeviceWaitIdle(device_->device());
}


// ===== 资源 / 场景接口（暂时 stub，后续接入 ResourceManager） =====

MeshHandle VulkanRenderer::createMesh(
    const void* vertexData,
    size_t      vertexCount,
    const void* indexData,
    size_t      indexCount)
{
    if (!vertexData || vertexCount == 0)
    {
        throw std::runtime_error("VulkanRenderer::createMesh: vertex data is null or empty");
    }

    // 1. 在 ResourceManager 中为顶点/索引缓冲分配 GPU 资源（仅创建，不上传）
    vkresource::MeshDesc desc{};
    desc.vertexBufferSize  = static_cast<VkDeviceSize>(vertexCount * sizeof(Vertex));
    desc.vertexUsage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    desc.vertexMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    desc.vertexDebugName   = "Mesh_VertexBuffer";

    if (indexData && indexCount > 0)
    {
        desc.indexBufferSize  = static_cast<VkDeviceSize>(indexCount * sizeof(uint32_t));
        desc.indexUsage       = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        desc.indexMemoryFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        desc.indexDebugName   = "Mesh_IndexBuffer";
    }

    vkresource::MeshHandle rmMesh = resourceManager_->createMesh(desc);

    // 2. 上传数据
    vkresource::BufferHandle vbHandle = resourceManager_->getMeshVertexBuffer(rmMesh);
    resourceManager_->uploadBuffer(
                                   vbHandle,
                                   vertexData,
                                   desc.vertexBufferSize,
                                   0);

    if (indexData && indexCount > 0)
    {
        vkresource::BufferHandle ibHandle = resourceManager_->getMeshIndexBuffer(rmMesh);
        if (ibHandle)
        {
            resourceManager_->uploadBuffer(
                                           ibHandle,
                                           indexData,
                                           desc.indexBufferSize,
                                           0);
        }
    }

    // 3. 在本地表中登记完整的 rmMesh（含 index + generation），返回 1-based 的 MeshHandle.id
    uint32_t slot = 0;
    if (!freeMeshSlots_.empty())
    {
        slot = freeMeshSlots_.back();
        freeMeshSlots_.pop_back();
        meshTable_[slot] = rmMesh;
    }
    else
    {
        slot = static_cast<uint32_t>(meshTable_.size());
        meshTable_.push_back(rmMesh);
    }

    MeshHandle handle{};
    handle.id = slot + 1; // 0 作为无效
    return handle;
}

void VulkanRenderer::destroyMesh(MeshHandle mesh)
{
    if (mesh.id == 0)
        return;

    uint32_t slot = mesh.id - 1;
    if (slot >= meshTable_.size())
        return;

    vkresource::MeshHandle& rmMesh = meshTable_[slot];
    if (!rmMesh) // index == UINT32_MAX，说明已经被释放
        return;

    resourceManager_->destroyMesh(rmMesh);

    // 标记此 slot 可复用
    rmMesh.index      = std::numeric_limits<uint32_t>::max();
    rmMesh.generation = 0;
    freeMeshSlots_.push_back(slot);
}

void VulkanRenderer::submitCamera(const CameraData& camera)
{
    currentCamera_ = camera;
    // TODO: 写入 camera UBO / push constants
}

void VulkanRenderer::submitRenderables(const Renderable* renderables, size_t count)
{
    currentFrameRenderables_.assign(renderables, renderables + count);
    // TODO: 在 recordCommandBuffer 中根据 renderables 做真正 drawIndexed
}

// ===== 初始化 & 重建 =====

void VulkanRenderer::createInstanceAndSurface(void* windowHandle)
{
    auto* glfwWindow = static_cast<GLFWwindow*>(windowHandle);
    if (!glfwWindow)
        throw std::runtime_error("VulkanRenderer::createInstanceAndSurface: windowHandle is null or invalid");

    vkinit::createInstance(instance_, glfwWindow);
    vkinit::createSurface(instance_, glfwWindow, surface_);
}

void VulkanRenderer::createDeviceAndQueues()
{
    VulkanDeviceConfig cfg{};
    cfg.requiredExtensions = deviceExtensions;
    // 根据需要启用特性，例如：cfg.requiredFeatures.samplerAnisotropy = VK_TRUE;

    device_ = std::make_unique<VulkanDevice>(instance_, surface_, cfg);
}

void VulkanRenderer::createResourceManagers()
{
    resourceManager_      = std::make_unique<vkresource::ResourceManager>(*device_);
    descriptorSetManager_ = std::make_unique<DescriptorSetManager>(*device_);
}

void VulkanRenderer::createSwapchain()
{
    swapchain_.destroy(); // 清旧资源

    QueueFamilyIndices indices{};
    indices.graphicsFamily = device_->graphicsQueue().familyIndex;
    indices.presentFamily  = device_->presentQueue().familyIndex;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    vkcmd::createCommandPool(device_->device(), indices, commandPool);

    swapchain_ = SwapchainResources(device_->device(), commandPool);

    vkswapchain::createSwapChain(device_->physicalDevice(),
                                 device_->device(),
                                 surface_,
                                 indices,
                                 swapchain_.swapchain,
                                 swapchain_.images,
                                 swapchain_.imageFormat,
                                 swapchain_.extent);

    vkswapchain::createImageViews(device_->device(),
                                  swapchain_.images,
                                  swapchain_.imageFormat,
                                  swapchain_.imageViews);
}

void VulkanRenderer::createRenderPass()
{
    vkpipeline::createRenderPass(device_->device(), swapchain_.imageFormat, swapchain_.renderPass);
}

void VulkanRenderer::createGraphicsPipeline()
{
    vkpipeline::createGraphicsPipeline(device_->device(),
                                       swapchain_.extent,
                                       swapchain_.renderPass,
                                       swapchain_.pipelineLayout,
                                       swapchain_.graphicsPipeline);
}

void VulkanRenderer::createFramebuffers()
{
    vkpipeline::createFramebuffers(device_->device(),
                                   swapchain_.imageViews,
                                   swapchain_.renderPass,
                                   swapchain_.extent,
                                   swapchain_.framebuffers);
}

void VulkanRenderer::allocateSwapchainCommandBuffers()
{
    // 重新分配 swapchain 级别的 command buffers，数量与 framebuffers 一致
    if (!swapchain_.commandBuffers.empty())
    {
        vkFreeCommandBuffers(swapchain_.device,
                             swapchain_.commandPool,
                             static_cast<uint32_t>(swapchain_.commandBuffers.size()),
                             swapchain_.commandBuffers.data());
        swapchain_.commandBuffers.clear();
    }

    vkcmd::createCommandBuffers(device_->device(),
                                swapchain_.commandPool,
                                swapchain_.framebuffers,
                                swapchain_.renderPass,
                                swapchain_.extent,
                                swapchain_.graphicsPipeline,
                                swapchain_.imageViews,
                                swapchain_.commandBuffers);
}

void VulkanRenderer::createFrameResources()
{
    allocateSwapchainCommandBuffers();

    const uint32_t frameCount = std::max<uint32_t>(1, MAX_FRAMES_IN_FLIGHT);

    framesInFlight_.clear();
    framesInFlight_.resize(frameCount);

    imagesInFlight_.clear();
    imagesInFlight_.resize(swapchain_.images.size(), VK_NULL_HANDLE);

    VkDevice device = device_->device();

    for (uint32_t i = 0; i < frameCount; ++i)
    {
        FrameResources& fr = framesInFlight_[i];

        fr.commandBuffer = (i < swapchain_.commandBuffers.size())
                               ? swapchain_.commandBuffers[i]
                               : VK_NULL_HANDLE;

        VkSemaphoreCreateInfo semInfo{};
        semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VK_CHECK(vkCreateSemaphore(device, &semInfo, nullptr, &fr.imageAvailableSemaphore));
        VK_CHECK(vkCreateSemaphore(device, &semInfo, nullptr, &fr.renderFinishedSemaphore));

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VK_CHECK(vkCreateFence(device, &fenceInfo, nullptr, &fr.inFlightFence));
    }

    currentFrameIndex_ = 0;
}

// ===== 销毁辅助 =====

void VulkanRenderer::destroyFrameResources()
{
    if (!device_)
        return;

    VkDevice device = device_->device();

    for (auto& fr : framesInFlight_)
    {
        if (fr.imageAvailableSemaphore != VK_NULL_HANDLE)
            vkDestroySemaphore(device, fr.imageAvailableSemaphore, nullptr);
        if (fr.renderFinishedSemaphore != VK_NULL_HANDLE)
            vkDestroySemaphore(device, fr.renderFinishedSemaphore, nullptr);
        if (fr.inFlightFence != VK_NULL_HANDLE)
            vkDestroyFence(device, fr.inFlightFence, nullptr);
    }

    framesInFlight_.clear();
    imagesInFlight_.clear();
}

void VulkanRenderer::destroySwapchain()
{
    swapchain_.destroy(); // SwapchainResources 已经负责清理所有关联对象
}

void VulkanRenderer::destroyDeviceAndSurface()
{
    if (device_)
        device_.reset();

    if (instance_ != VK_NULL_HANDLE && surface_ != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::destroyInstance()
{
    if (instance_ != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}

// ===== 命令缓冲录制 =====

void VulkanRenderer::recordCommandBuffer(FrameResources& frame, uint32_t imageIndex)
{
    if (frame.commandBuffer == VK_NULL_HANDLE)
        return;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    VK_CHECK(vkBeginCommandBuffer(frame.commandBuffer, &beginInfo));

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = swapchain_.renderPass;
    renderPassInfo.framebuffer       = swapchain_.framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchain_.extent;

    VkClearValue clearColor{};
    clearColor.color               = {{0.0f, 0.0f, 0.0f, 1.0f}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues    = &clearColor;

    vkCmdBeginRenderPass(frame.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, swapchain_.graphicsPipeline);

    // 动态 viewport / scissor，和 Rendering.cpp 中的 pipeline 配置一致
    VkViewport viewport{};
    viewport.x        = 0.0f;
    viewport.y        = 0.0f;
    viewport.width    = static_cast<float>(swapchain_.extent.width);
    viewport.height   = static_cast<float>(swapchain_.extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(frame.commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchain_.extent;
    vkCmdSetScissor(frame.commandBuffer, 0, 1, &scissor);

    vkCmdSetLineWidth(frame.commandBuffer, 1.0f);
    vkCmdSetDepthBias(frame.commandBuffer, 0.0f, 0.0f, 0.0f);

    // 根据当前帧的 renderables 绘制 mesh
    for (const auto& r : currentFrameRenderables_)
    {
        if (r.mesh.id == 0)
            continue;

        uint32_t slot = r.mesh.id - 1;
        if (slot >= meshTable_.size())
            continue;

        const vkresource::MeshHandle& rmMesh = meshTable_[slot];
        if (!rmMesh) // 已被 destroy
            continue;

        vkresource::BufferHandle vbHandle = resourceManager_->getMeshVertexBuffer(rmMesh);
        vkresource::BufferHandle ibHandle = resourceManager_->getMeshIndexBuffer(rmMesh);
        const auto&              meshDesc = resourceManager_->getMeshDesc(rmMesh);

        VkBuffer vb = resourceManager_->getBuffer(vbHandle);
        if (vb == VK_NULL_HANDLE)
            continue;

        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(frame.commandBuffer, 0, 1, &vb, offsets);

        // 有索引缓冲则 indexed 绘制
        if (ibHandle)
        {
            VkBuffer ib = resourceManager_->getBuffer(ibHandle);
            if (ib == VK_NULL_HANDLE)
                continue;

            vkCmdBindIndexBuffer(frame.commandBuffer, ib, 0, VK_INDEX_TYPE_UINT32);

            uint32_t indexCount =
                    meshDesc.indexBufferSize > 0
                        ? static_cast<uint32_t>(meshDesc.indexBufferSize / sizeof(uint32_t))
                        : 0;
            if (indexCount == 0)
                continue;

            vkCmdDrawIndexed(frame.commandBuffer, indexCount, 1, 0, 0, 0);
        }
        else
        {
            uint32_t vertexCount =
                    meshDesc.vertexBufferSize > 0
                        ? static_cast<uint32_t>(meshDesc.vertexBufferSize / sizeof(Vertex))
                        : 0;
            if (vertexCount == 0)
                continue;

            vkCmdDraw(frame.commandBuffer, vertexCount, 1, 0, 0);
        }
    }

    vkCmdEndRenderPass(frame.commandBuffer);

    VK_CHECK(vkEndCommandBuffer(frame.commandBuffer));
}

#include "VulkanRenderer.h"
#include "command_buffer_sync.h"
#include "constants.h"
#include "Platform.h"
#include "Rendering.h"
#include "swapchain_management.h"
#include "utils.h"
#include "vulkan_init.h"

// ======================== 构造 / 析构 ========================


VulkanRenderer::~VulkanRenderer()
{
    waitIdle();

    // 先销毁依赖 device/swapchain 的高层资源
    destroyFrameResources();
    destroyFramebuffers();
    destroyGraphicsPipeline();
    destroyRenderPass();
    destroySwapchain();

    destroyDeviceAndSurface();
    destroyInstance();
}

// ======================== Renderer 接口实现 ========================

void VulkanRenderer::initialize(void* windowHandle, int width, int height)
{
    // 典型初始化顺序：
    // 1. Instance + Surface
    // 2. Device + Queues
    // 3. 资源管理子系统（ResourceManager / DescriptorSetManager）
    // 4. Swapchain
    // 5. RenderPass
    // 6. Pipeline
    // 7. Framebuffers
    // 8. FrameResources

    createInstanceAndSurface(windowHandle);
    createDeviceAndQueues();
    createResourceManagers();

    createSwapchain();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createFrameResources();

    // 初始状态
    currentFrameIndex_  = 0;
    currentImageIndex_  = 0;
    swapchainOutOfDate_ = false;
}

void VulkanRenderer::resize(int width, int height)
{
    // 建议：上层判断 width/height==0 时暂不调用 resize（窗口最小化）
    if (width == 0 || height == 0)
        return;

    waitIdle();

    destroyFrameResources();
    destroyFramebuffers();
    destroyGraphicsPipeline();
    destroyRenderPass();
    destroySwapchain();

    createSwapchain();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createFrameResources();

    // Reset frame state after resize
    currentFrameIndex_  = 0;
    currentImageIndex_  = 0;
    swapchainOutOfDate_ = false;
}

bool VulkanRenderer::beginFrame(const FrameContext& /*ctx*/)
{
    // 典型流程：
    // 1. 等待当前帧 fence 完成
    // 2. acquire swapchain image → currentImageIndex_
    // 3. 如果这张 image 还被别的帧在用，等它的 fence

    if (framesInFlight_.empty() || !device_ || swapchainOutOfDate_)
        return false;

    FrameResources& frame = framesInFlight_[currentFrameIndex_];

    // 1) 等待当前帧 fence，确保上一轮提交已完成
    vkWaitForFences(device_->device(), 1, &frame.inFlightFence, VK_TRUE, UINT64_MAX);

    // 2) 获取下一个 swapchain image
    uint32_t imageIndex = 0;
    VkResult result     = vkAcquireNextImageKHR(
                                                device_->device(),
                                                swapchain_.swapchain,
                                                UINT64_MAX,
                                                frame.imageAvailableSemaphore,
                                                // acquire 完成后 signal 这个 semaphore
                                                VK_NULL_HANDLE,
                                                &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // 交给外层 resize
        swapchainOutOfDate_ = true;
        return false;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
        throw std::runtime_error("VulkanRenderer::beginFrame - failed to acquire swap chain image");
    }

    // CRITICAL: Reset fence only AFTER successful image acquisition.
    // If we reset before checking acquisition result and then return early due to error,
    // the next frame will wait forever on this unsignaled fence, causing a hang.
    vkResetFences(device_->device(), 1, &frame.inFlightFence);

    // 3) 如果这张 image 之前被别的帧使用过，等那个帧的 fence，避免同一 image 并发 in-flight
    if (imageIndex < imagesInFlight_.size() && imagesInFlight_[imageIndex] != VK_NULL_HANDLE)
    {
        vkWaitForFences(device_->device(), 1, &imagesInFlight_[imageIndex], VK_TRUE, UINT64_MAX);
    }

    // 将这张 image 标记为由当前帧的 fence 保护
    if (imageIndex < imagesInFlight_.size())
    {
        imagesInFlight_[imageIndex] = frame.inFlightFence;
    }

    currentImageIndex_ = imageIndex;
    return true;
}


void VulkanRenderer::renderFrame()
{
    if (framesInFlight_.empty() || !device_ || swapchainOutOfDate_)
        return;

    FrameResources& frame = framesInFlight_[currentFrameIndex_];

    // 1) 重置 command buffer
    if (frame.commandBuffer != VK_NULL_HANDLE)
    {
        vkResetCommandBuffer(frame.commandBuffer, 0);
    }

    // 2) 录制命令
    recordCommandBuffer(frame, currentImageIndex_);

    // 3) 提交到 graphics queue
    VkSemaphore          waitSemaphores[]   = {frame.imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[]       = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSemaphore          signalSemaphores[] = {frame.renderFinishedSemaphore};

    VkSubmitInfo submitInfo{};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pWaitSemaphores      = waitSemaphores;
    submitInfo.pWaitDstStageMask    = waitStages;
    submitInfo.commandBufferCount   = 1;
    submitInfo.pCommandBuffers      = &frame.commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    if (vkQueueSubmit(device_->graphicsQueue().handle, 1, &submitInfo, frame.inFlightFence) != VK_SUCCESS)
    {
        throw std::runtime_error("VulkanRenderer::renderFrame - failed to submit draw command buffer");
    }

    // 4) present
    VkSwapchainKHR swapchains[] = {swapchain_.swapchain};

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores; // 等 renderFinishedSemaphore
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = swapchains;
    presentInfo.pImageIndices      = &currentImageIndex_;

    VkResult result = vkQueuePresentKHR(device_->presentQueue().handle, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        swapchainOutOfDate_ = true;
    }
    else if (result != VK_SUCCESS)
    {
        throw std::runtime_error("VulkanRenderer::renderFrame - failed to present swap chain image");
    }

    // 5) 下一帧使用下一个 FrameResources
    currentFrameIndex_ = (currentFrameIndex_ + 1) % MAX_FRAMES_IN_FLIGHT;
}

void VulkanRenderer::waitIdle()
{
    if (!device_)
        return;

    VkDevice dev = device_->device();
    if (dev != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(dev);
    }
}

// ======================== 资源 & 场景接口 ========================

MeshHandle VulkanRenderer::createMesh(
    const void* /*vertexData*/,
    size_t /*vertexCount*/,
    const void* /*indexData*/,
    size_t /*indexCount*/)
{
    // TODO: 2.x 阶段：
    // 1. 使用 ResourceManager 创建 vertex/index buffer（含 staging 上传）
    // 2. 返回一个 MeshHandle（内部可为资源表索引或句柄对象）
    return MeshHandle{0};
}

void VulkanRenderer::destroyMesh(MeshHandle /*mesh*/)
{
    // TODO: 调用 ResourceManager 释放对应 GPU 资源
}

void VulkanRenderer::submitCamera(const CameraData& camera)
{
    currentCamera_ = camera;
}

void VulkanRenderer::submitRenderables(const Renderable* renderables, size_t count)
{
    currentFrameRenderables_.assign(renderables, renderables + count);
}

// ======================== 初始化 & 重建（私有） ========================

void VulkanRenderer::createInstanceAndSurface(void* windowHandle)
{
    // 说明：
    // - 这里假设用 GLFW 创建窗口，windowHandle 为 GLFWwindow*
    // - 你可以选择：由 VulkanRenderer 拥有 instance/surface，或从外部注入，这里按“拥有”处理

    auto* window = static_cast<GLFWwindow*>(windowHandle);

    // 实例
    createInstance(instance_, window); // 来自 vulkan_init.cpp

    setupDebugMessenger(instance_);
    // 表面
    createSurface(instance_, window, surface_); // 来自 vulkan_init.cpp
}

void VulkanRenderer::createDeviceAndQueues()
{
    // 使用 VulkanDevice 封装设备创建：
    VulkanDeviceConfig config{};
    config.requiredExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    // 如需 wideLines / samplerAnisotropy 等特性可在此赋值：
    // config.requiredFeatures.samplerAnisotropy = VK_TRUE;
    device_ = std::make_unique<VulkanDevice>(instance_, surface_, config);
}

void VulkanRenderer::createResourceManagers()
{
    resourceManager_      = std::make_unique<ResourceManager>(*device_);
    descriptorSetManager_ = std::make_unique<DescriptorSetManager>(*device_);
}


void VulkanRenderer::createSwapchain()
{
    // 职责：只负责创建 swapchain_ 的核心数据（swapchain / images / format / extent / views）

    // 若 SwapchainResources 需要 commandPool，可在此创建
    if (swapchain_.device == VK_NULL_HANDLE)
    {
        // 主命令池：使用 graphics 队列族
        VkCommandPool commandPool =
                device_->createCommandPool(device_->graphicsQueue().familyIndex,
                                           VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        swapchain_ = SwapchainResources(device_->device(), commandPool);
    }

    // 填充 QueueFamilyIndices 供 createSwapChain 使用
    QueueFamilyIndices indices{};
    indices.graphicsFamily = device_->graphicsQueue().familyIndex;
    indices.presentFamily  = device_->presentQueue().familyIndex;

    // 创建 swapchain / images / format / extent
    createSwapChain(device_->physicalDevice(),
                    device_->device(),
                    surface_,
                    indices,
                    swapchain_.swapchain,
                    swapchain_.images,
                    swapchain_.imageFormat,
                    swapchain_.extent);

    // 创建 image views
    createImageViews(device_->device(), swapchain_.images, swapchain_.imageFormat, swapchain_.imageViews);
}

void VulkanRenderer::createRenderPass()
{
    // 使用 rendering.cpp 中的 createRenderPass 工具
    ::createRenderPass(device_->device(), swapchain_.imageFormat, renderPass_);
}

void VulkanRenderer::createGraphicsPipeline()
{
    // 使用 rendering.cpp 中的 createGraphicsPipeline 工具
    ::createGraphicsPipeline(device_->device(),
                             swapchain_.extent,
                             renderPass_,
                             pipelineLayout_,
                             graphicsPipeline_);
}

void VulkanRenderer::createFramebuffers()
{
    // 基于 swapchain_.imageViews + renderPass_ 创建 framebuffers
    VkDevice device = device_->device();

    swapchain_.framebuffers.resize(swapchain_.imageViews.size());

    for (size_t i = 0; i < swapchain_.imageViews.size(); ++i)
    {
        VkImageView attachments[] = {swapchain_.imageViews[i]};

        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass      = renderPass_;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments    = attachments;
        fbInfo.width           = swapchain_.extent.width;
        fbInfo.height          = swapchain_.extent.height;
        fbInfo.layers          = 1;

        if (vkCreateFramebuffer(device, &fbInfo, nullptr, &swapchain_.framebuffers[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("VulkanRenderer::createFramebuffers - failed to create framebuffer");
        }
    }
}

void VulkanRenderer::createFrameResources()
{
    destroyFrameResources();

    VkDevice      device = device_->device();
    VkCommandPool pool   = swapchain_.commandPool;
    if (pool == VK_NULL_HANDLE)
    {
        QueueFamilyIndices      indices = findQueueFamilies(device_->physicalDevice(), surface_);
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = indices.graphicsFamily.value();
        if (vkCreateCommandPool(device, &poolInfo, nullptr, &pool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create command pool for frames in flight");
        }
        swapchain_.commandPool = pool;
        swapchain_.device      = device;
    }

    framesInFlight_.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // 初始 signaled，第一帧不会卡在 Wait 上
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (auto& frame : framesInFlight_)
    {
        // 1) command buffer
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool        = pool;
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(device, &allocInfo, &frame.commandBuffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate command buffer for frame");
        }

        // 2) per-frame semaphores
        if (vkCreateSemaphore(device, &semInfo, nullptr, &frame.imageAvailableSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semInfo, nullptr, &frame.renderFinishedSemaphore) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create semaphores for frame");
        }

        // 3) in-flight fence
        if (vkCreateFence(device, &fenceInfo, nullptr, &frame.inFlightFence) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create in-flight fence for frame");
        }
    }

    // 每张 image 当前对应哪个 fence，初始都没有
    imagesInFlight_.assign(swapchain_.images.size(), VK_NULL_HANDLE);
}

void VulkanRenderer::destroyFrameResources()
{
    VkDevice device = device_ ? device_->device() : VK_NULL_HANDLE;
    if (device == VK_NULL_HANDLE)
    {
        framesInFlight_.clear();
        imagesInFlight_.clear();
        return;
    }

    for (auto& frame : framesInFlight_)
    {
        if (frame.inFlightFence)
        {
            vkDestroyFence(device, frame.inFlightFence, nullptr);
            frame.inFlightFence = VK_NULL_HANDLE;
        }
        if (frame.imageAvailableSemaphore)
        {
            vkDestroySemaphore(device, frame.imageAvailableSemaphore, nullptr);
            frame.imageAvailableSemaphore = VK_NULL_HANDLE;
        }
        if (frame.renderFinishedSemaphore)
        {
            vkDestroySemaphore(device, frame.renderFinishedSemaphore, nullptr);
            frame.renderFinishedSemaphore = VK_NULL_HANDLE;
        }
        if (frame.commandBuffer)
        {
            vkFreeCommandBuffers(device, swapchain_.commandPool, 1, &frame.commandBuffer);
            frame.commandBuffer = VK_NULL_HANDLE;
        }
    }

    framesInFlight_.clear();
    imagesInFlight_.clear();
}


void VulkanRenderer::destroyFramebuffers()
{
    VkDevice device = device_->device();

    for (auto fb : swapchain_.framebuffers)
    {
        if (fb)
            vkDestroyFramebuffer(device, fb, nullptr);
    }
    swapchain_.framebuffers.clear();
}

void VulkanRenderer::destroyGraphicsPipeline()
{
    VkDevice device = device_->device();

    if (graphicsPipeline_)
    {
        vkDestroyPipeline(device, graphicsPipeline_, nullptr);
        graphicsPipeline_ = VK_NULL_HANDLE;
    }
    if (pipelineLayout_)
    {
        vkDestroyPipelineLayout(device, pipelineLayout_, nullptr);
        pipelineLayout_ = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::destroyRenderPass()
{
    VkDevice device = device_->device();

    if (renderPass_)
    {
        vkDestroyRenderPass(device, renderPass_, nullptr);
        renderPass_ = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::destroySwapchain()
{
    // SwapchainResources::destroy 会统一销毁 swapchain / views / framebuffers / command buffers
    swapchain_.destroy();
}

void VulkanRenderer::destroyDeviceAndSurface()
{
    // 由 VulkanDevice 析构函数负责（或外部管理）vkDestroyDevice，这里只需释放智能指针
    // 注意：surface 的销毁依赖 instance，在 destroyInstance 中处理
    device_.reset();
}

void VulkanRenderer::destroyInstance()
{
    // 先销毁 surface，再销毁 instance
    if (surface_ != VK_NULL_HANDLE && instance_ != VK_NULL_HANDLE)
    {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }

    if (instance_ != VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}

// ======================== 帧录制（私有） ========================

void VulkanRenderer::recordCommandBuffer(FrameResources& frame, uint32_t imageIndex)
{
    // 职责：仅封装命令录制逻辑，不做同步处理

    if (frame.commandBuffer == VK_NULL_HANDLE)
        return;

    // 从 swapchain_.framebuffers 中选择对应 framebuffer
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    if (imageIndex < swapchain_.framebuffers.size())
    {
        framebuffer = swapchain_.framebuffers[imageIndex];
    }

    // 复用你现有的 recordCommandBuffer 工具函数（command_buffer_sync.cpp）
    ::recordCommandBuffer(frame.commandBuffer,
                          imageIndex,
                          renderPass_,
                          swapchain_.extent,
                          graphicsPipeline_,
                          framebuffer);

    // TODO：未来在这里插入：
    // - 绑定 vertex/index buffer（使用 MeshHandle -> ResourceManager）
    // - 绑定 descriptor sets（使用 DescriptorSetManager）
    // - 多 pass 渲染调用
}

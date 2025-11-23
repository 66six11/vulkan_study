//
// Created by C66 on 2025/11/23.
//
#pragma once
#ifndef VULKAN_VULKANRENDERER_H
#define VULKAN_VULKANRENDERER_H

#include "DescriptorSetManager.h"
#include "Renderer.h"
#include "ResourceManager.h"
#include "SwapchainResources.h" // 或你当前的 swapchain 封装头
#include "VulkanDevice.h"

// 可选：Platform.h，里面 typedef/using 平台窗口句柄


class VulkanRenderer : public Renderer
{
    public:
        VulkanRenderer() = default;
        ~VulkanRenderer() override;

        void initialize(void* windowHandle, int width, int height) override;
        void resize(int width, int height) override;

        bool beginFrame(const FrameContext& ctx) override;
        void renderFrame() override;
        void waitIdle() override;

        MeshHandle createMesh(
            const void* vertexData,
            size_t      vertexCount,
            const void* indexData,
            size_t      indexCount) override;

        void destroyMesh(MeshHandle mesh) override;
        void submitCamera(const CameraData& camera) override;
        void submitRenderables(const Renderable* renderables, size_t count) override;

    private:
        // 1. 核心 Vulkan 句柄
        VkInstance                    instance_ = VK_NULL_HANDLE;
        VkSurfaceKHR                  surface_  = VK_NULL_HANDLE;
        std::unique_ptr<VulkanDevice> device_;

        // 2. 资源管理
        std::unique_ptr<ResourceManager>      resourceManager_;
        std::unique_ptr<DescriptorSetManager> descriptorSetManager_;

        // 3. 交换链及其相关资源（RAII）
        SwapchainResources swapchain_;

        // 4. Per-frame 同步与命令缓冲绑定
        struct FrameResources
        {
            VkCommandBuffer commandBuffer           = VK_NULL_HANDLE;
            VkSemaphore     imageAvailableSemaphore = VK_NULL_HANDLE;
            VkSemaphore     renderFinishedSemaphore = VK_NULL_HANDLE;
            VkFence         inFlightFence           = VK_NULL_HANDLE;
        };

        std::vector<FrameResources> framesInFlight_;
        std::vector<VkFence>        imagesInFlight_; // per-swapchain-image fence

        uint32_t currentFrameIndex_ = 0;
        uint32_t currentImageIndex_ = 0;

        // 5. 场景数据缓存
        CameraData              currentCamera_;
        std::vector<Renderable> currentFrameRenderables_;

        bool swapchainOutOfDate_ = false;

        // Init/recreate helpers
        void createInstanceAndSurface(void* windowHandle);
        void createDeviceAndQueues();
        void createResourceManagers();
        void createSwapchain();
        void createRenderPass();
        void createGraphicsPipeline();
        void createFramebuffers();
        void allocateSwapchainCommandBuffers();
        void createFrameResources();

        // Destroy helpers
        void destroyFrameResources();
        void destroySwapchain();
        void destroyDeviceAndSurface();
        void destroyInstance();

        // Recording
        void recordCommandBuffer(FrameResources& frame, uint32_t imageIndex);
};

#endif //VULKAN_VULKANRENDERER_H

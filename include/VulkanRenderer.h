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

        // 资源 & 场景接口
        MeshHandle createMesh(
            const void* vertexData,
            size_t      vertexCount,
            const void* indexData,
            size_t      indexCount) override;
        void destroyMesh(MeshHandle mesh) override;
        void submitCamera(const CameraData& camera) override;
        void submitRenderables(const Renderable* renderables, size_t count) override;

    private:
        // 1. 核心 Vulkan 对象（设备层）
        VkInstance   instance_ = VK_NULL_HANDLE; // 如果 Renderer 拥有 instance
        VkSurfaceKHR surface_  = VK_NULL_HANDLE;

        std::unique_ptr<VulkanDevice> device_; // 包含 VkDevice / VkPhysicalDevice / 队列信息

        // 2. 资源管理子系统（为未来做准备）
        std::unique_ptr<ResourceManager>      resourceManager_;
        std::unique_ptr<DescriptorSetManager> descriptorSetManager_;

        // 3. 交换链层
        SwapchainResources swapchain_; // RAII 包含 swapchain / views / framebuffers / command buffers

        // 4. pipeline & render pass 层
        VkRenderPass     renderPass_       = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout_   = VK_NULL_HANDLE;
        VkPipeline       graphicsPipeline_ = VK_NULL_HANDLE;

        // 5. per-frame 资源层（帧同步 + 命令缓冲）

        struct FrameResources
        {
            VkCommandBuffer commandBuffer           = VK_NULL_HANDLE;
            VkSemaphore     imageAvailableSemaphore = VK_NULL_HANDLE;
            VkSemaphore     renderFinishedSemaphore = VK_NULL_HANDLE;
            VkFence         inFlightFence           = VK_NULL_HANDLE;
        };

        std::vector<FrameResources> framesInFlight_;
        uint32_t                    currentFrameIndex_ = 0;
        uint32_t                    currentImageIndex_ = 0;
        // 每个 image 当前关联的 inFlightFence（哪一帧在用它）
        std::vector<VkFence> imagesInFlight_;
        // 6. 场景数据缓存层
        CameraData              currentCamera_;
        std::vector<Renderable> currentFrameRenderables_;

        // 7. 状态/标志
        bool swapchainOutOfDate_ = false; // beginFrame / renderFrame 中设置，外部驱动 resize


        // ========= 初始化 & 重建 =========
        void createInstanceAndSurface(void* windowHandle); // 如果不由外部注入
        void createDeviceAndQueues();                      // 用 VulkanDeviceConfig 选择设备
        void createResourceManagers();                     // ResourceManager + DescriptorSetManager
        void createSwapchain();                            // 使用 swapchain_management 工具
        void createRenderPass();                           // 使用 rendering.cpp 的 createRenderPass
        void createGraphicsPipeline();                     // 使用 rendering.cpp 的 createGraphicsPipeline
        void createFramebuffers();                         // 基于 swapchain image views + renderPass_
        void createFrameResources();                       // per-frame semaphores/fences/command buffers

        void destroyFrameResources();
        void destroyFramebuffers();
        void destroyGraphicsPipeline();
        void destroyRenderPass();
        void destroySwapchain();
        void destroyDeviceAndSurface();
        void destroyInstance();

        // ========= 帧录制 =========
        void recordCommandBuffer(FrameResources& frame, uint32_t imageIndex);

        // ========= 资源/场景 =========
        // 这里可以再细分成 Mesh 创建/Material 创建等私有 helper，用 ResourceManager 封装
};
#endif //VULKAN_VULKANRENDERER_H

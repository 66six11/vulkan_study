#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>  // For VkDebugUtilsMessengerEXT, VkSurfaceKHR

#include "Buffer.hpp"
#include "CommandBuffer.hpp"
#include "Core.hpp"
#include "Pipeline.hpp"
#include "SwapChain.hpp"
#include "Texture.hpp"

namespace engine::rhi
{
    // Device description
    struct DeviceDesc
    {
        void*    platformWindowHandle     = nullptr; // Platform-specific window handle
        Extent2D initialSwapChainExtent   = {1280, 720};
        Format   preferredSwapChainFormat = Format::B8G8R8A8_UNORM;
        bool     enableValidation         = true;
        bool     enableDebugMarkers       = true;
        bool     preferDiscreteGPU        = true;
        // Device features can be added here
    };

    // Submit info
    struct SubmitInfo
    {
        std::vector<CommandBufferHandle> commandBuffers;
        std::vector<SemaphoreHandle>     waitSemaphores;
        std::vector<PipelineStage>       waitStages;
        std::vector<SemaphoreHandle>     signalSemaphores;
    };

    // Device class - main interface for RHI
    class Device
    {
        public:
            Device();
            ~Device();

            Device(const Device&)            = delete;
            Device& operator=(const Device&) = delete;

            Device(Device&& other) noexcept;
            Device& operator=(Device&& other) noexcept;

            // Initialization
            [[nodiscard]] Result initialize(const DeviceDesc& desc);
            void                 shutdown();
            [[nodiscard]] bool   isInitialized() const noexcept { return initialized_; }

            // Resource creation
            [[nodiscard]] ResultValue<BufferHandle>      createBuffer(const BufferDesc& desc);
            [[nodiscard]] ResultValue<TextureHandle>     createTexture(const TextureDesc& desc);
            [[nodiscard]] ResultValue<TextureViewHandle> createTextureView(
                TextureHandle          texture,
                const TextureViewDesc& desc);
            [[nodiscard]] ResultValue<SamplerHandle>             createSampler(const SamplerDesc& desc);
            [[nodiscard]] ResultValue<ShaderHandle>              createShader(const ShaderDesc& desc);
            [[nodiscard]] ResultValue<DescriptorSetLayoutHandle> createDescriptorSetLayout(
                const DescriptorSetLayoutDesc& desc);
            [[nodiscard]] ResultValue<PipelineLayoutHandle> createPipelineLayout(
                const PipelineLayoutDesc& desc);
            [[nodiscard]] ResultValue<GraphicsPipelineHandle> createGraphicsPipeline(
                const GraphicsPipelineDesc& desc);
            [[nodiscard]] ResultValue<ComputePipelineHandle> createComputePipeline(
                const ComputePipelineDesc& desc);
            [[nodiscard]] ResultValue<DescriptorSetHandle> allocateDescriptorSet(
                DescriptorSetLayoutHandle layout);
            [[nodiscard]] ResultValue<DescriptorSetHandle> allocateDescriptorSet(
                DescriptorSetLayoutHandle                              layout,
                const std::vector<DescriptorSetHandle::element_type*>& resources);

            // Swap chain
            [[nodiscard]] ResultValue<SwapChainHandle> createSwapChain(const SwapChainDesc& desc);
            void                                       destroySwapChain(SwapChainHandle swapChain);

            // Command buffer allocation
            [[nodiscard]] ResultValue<CommandBufferHandle> allocateCommandBuffer(QueueType queueType);
            [[nodiscard]] std::vector<CommandBufferHandle> allocateCommandBuffers(QueueType queueType, uint32_t count);
            void                                           freeCommandBuffer(CommandBufferHandle cmd);
            void                                           freeCommandBuffers(const std::vector<CommandBufferHandle>& cmds);
            void                                           resetCommandPool(QueueType queueType);

            // Synchronization primitives
            [[nodiscard]] ResultValue<SemaphoreHandle> createSemaphore();
            [[nodiscard]] ResultValue<FenceHandle>     createFence(bool signaled = false);
            void                                       destroySemaphore(SemaphoreHandle semaphore);
            void                                       destroyFence(FenceHandle fence);

            // Submit and present
            [[nodiscard]] Result submit(
                QueueType         queueType,
                const SubmitInfo& info,
                FenceHandle       fence = nullptr);
            [[nodiscard]] Result present(
                SwapChainHandle                     swapChain,
                const std::vector<SemaphoreHandle>& waitSemaphores);

            // Wait operations
            [[nodiscard]] Result            waitForFence(FenceHandle fence, uint64_t timeout = UINT64_MAX);
            [[nodiscard]] Result            resetFence(FenceHandle fence);
            [[nodiscard]] ResultValue<bool> isFenceSignaled(FenceHandle fence);
            [[nodiscard]] Result            waitIdle();

            // Garbage collection
            void runGarbageCollection();

            // Queries
            [[nodiscard]] const DeviceCapabilities& capabilities() const noexcept { return capabilities_; }
            [[nodiscard]] uint32_t                  currentFrameIndex() const noexcept { return currentFrameIndex_; }
            [[nodiscard]] void*                     nativeDevice() const noexcept { return nativeDevice_; }
            [[nodiscard]] void*                     nativePhysicalDevice() const noexcept { return nativePhysicalDevice_; }
            [[nodiscard]] void*                     nativeInstance() const noexcept { return nativeInstance_; }
            [[nodiscard]] void*                     nativeVmaAllocator() const noexcept { return vmaAllocator_; }

            // Queue families (for advanced usage)
            [[nodiscard]] uint32_t graphicsQueueFamily() const noexcept { return graphicsQueueFamily_; }
            [[nodiscard]] uint32_t computeQueueFamily() const noexcept { return computeQueueFamily_; }
            [[nodiscard]] uint32_t transferQueueFamily() const noexcept { return transferQueueFamily_; }

        private:
            void release();

            // Vulkan handles
            VkInstance       nativeInstance_       = nullptr;
            VkPhysicalDevice nativePhysicalDevice_ = nullptr;
            VkDevice         nativeDevice_         = nullptr;
            VmaAllocator     vmaAllocator_         = nullptr;

            // Queues
            VkQueue graphicsQueue_ = nullptr;
            VkQueue computeQueue_  = nullptr;
            VkQueue transferQueue_ = nullptr;

            // Queue families
            uint32_t graphicsQueueFamily_ = UINT32_MAX;
            uint32_t computeQueueFamily_  = UINT32_MAX;
            uint32_t transferQueueFamily_ = UINT32_MAX;

            // Command pools
            VkCommandPool graphicsCommandPool_ = nullptr;
            VkCommandPool computeCommandPool_  = nullptr;
            VkCommandPool transferCommandPool_ = nullptr;

            // Debug messenger
            VkDebugUtilsMessengerEXT debugMessenger_ = nullptr;

            // State
            bool                      initialized_ = false;
            DeviceCapabilities        capabilities_;
            uint32_t                  currentFrameIndex_   = 0;
            static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 3;

            // Deletion queue
            struct DeletionEntry
            {
                uint32_t              frameIndex;
                std::function<void()> deleter;
            };

            std::vector<DeletionEntry> deletionQueue_;

            // Helper functions
            [[nodiscard]] Result createInstance(bool enableValidation);
            [[nodiscard]] Result selectPhysicalDevice(bool preferDiscrete);
            [[nodiscard]] Result createLogicalDevice();
            [[nodiscard]] Result createCommandPools();
            [[nodiscard]] Result createVmaAllocator();
            void                 queryCapabilities();

            // Platform-specific surface creation
            [[nodiscard]] ResultValue<VkSurfaceKHR> createSurface(void* windowHandle);
    };

    using DeviceHandle = std::shared_ptr<Device>;
} // namespace engine::rhi

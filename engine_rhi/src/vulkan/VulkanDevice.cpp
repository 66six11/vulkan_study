#include "engine/rhi/Device.hpp"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <vector>
#include <set>
#include <algorithm>

namespace engine::rhi
{
    // Debug callback
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT             messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void*                                       pUserData)
    {
        (void)messageType;
        (void)pUserData;

        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            // Log warning/error
        }

        return VK_FALSE;
    }

    Device::Device() = default;

    Device::~Device()
    {
        shutdown();
    }

    Device::Device(Device&& other) noexcept
        : nativeInstance_(other.nativeInstance_)
        , nativePhysicalDevice_(other.nativePhysicalDevice_)
        , nativeDevice_(other.nativeDevice_)
        , vmaAllocator_(other.vmaAllocator_)
        , graphicsQueue_(other.graphicsQueue_)
        , computeQueue_(other.computeQueue_)
        , transferQueue_(other.transferQueue_)
        , graphicsQueueFamily_(other.graphicsQueueFamily_)
        , computeQueueFamily_(other.computeQueueFamily_)
        , transferQueueFamily_(other.transferQueueFamily_)
        , graphicsCommandPool_(other.graphicsCommandPool_)
        , computeCommandPool_(other.computeCommandPool_)
        , transferCommandPool_(other.transferCommandPool_)
        , debugMessenger_(other.debugMessenger_)
        , initialized_(other.initialized_)
        , capabilities_(other.capabilities_)
        , currentFrameIndex_(other.currentFrameIndex_)
        , deletionQueue_(std::move(other.deletionQueue_))
    {
        other.nativeInstance_       = nullptr;
        other.nativePhysicalDevice_ = nullptr;
        other.nativeDevice_         = nullptr;
        other.vmaAllocator_         = nullptr;
        other.graphicsQueue_        = nullptr;
        other.computeQueue_         = nullptr;
        other.transferQueue_        = nullptr;
        other.graphicsCommandPool_  = nullptr;
        other.computeCommandPool_   = nullptr;
        other.transferCommandPool_  = nullptr;
        other.debugMessenger_       = nullptr;
        other.initialized_          = false;
    }

    Device& Device::operator=(Device&& other) noexcept
    {
        if (this != &other)
        {
            shutdown();

            nativeInstance_       = other.nativeInstance_;
            nativePhysicalDevice_ = other.nativePhysicalDevice_;
            nativeDevice_         = other.nativeDevice_;
            vmaAllocator_         = other.vmaAllocator_;
            graphicsQueue_        = other.graphicsQueue_;
            computeQueue_         = other.computeQueue_;
            transferQueue_        = other.transferQueue_;
            graphicsQueueFamily_  = other.graphicsQueueFamily_;
            computeQueueFamily_   = other.computeQueueFamily_;
            transferQueueFamily_  = other.transferQueueFamily_;
            graphicsCommandPool_  = other.graphicsCommandPool_;
            computeCommandPool_   = other.computeCommandPool_;
            transferCommandPool_  = other.transferCommandPool_;
            debugMessenger_       = other.debugMessenger_;
            initialized_          = other.initialized_;
            capabilities_         = other.capabilities_;
            currentFrameIndex_    = other.currentFrameIndex_;
            deletionQueue_        = std::move(other.deletionQueue_);

            other.nativeInstance_       = nullptr;
            other.nativePhysicalDevice_ = nullptr;
            other.nativeDevice_         = nullptr;
            other.vmaAllocator_         = nullptr;
            other.graphicsQueue_        = nullptr;
            other.computeQueue_         = nullptr;
            other.transferQueue_        = nullptr;
            other.graphicsCommandPool_  = nullptr;
            other.computeCommandPool_   = nullptr;
            other.transferCommandPool_  = nullptr;
            other.debugMessenger_       = nullptr;
            other.initialized_          = false;
        }
        return *this;
    }

    Result Device::initialize(const DeviceDesc& desc)
    {
        if (initialized_)
        {
            return Result::Error_InvalidParameter;
        }

        // Create instance
        Result result = createInstance(desc.enableValidation);
        if (result != Result::Success)
        {
            return result;
        }

        // Select physical device
        result = selectPhysicalDevice(desc.preferDiscreteGPU);
        if (result != Result::Success)
        {
            shutdown();
            return result;
        }

        // Create logical device
        result = createLogicalDevice();
        if (result != Result::Success)
        {
            shutdown();
            return result;
        }

        // Create command pools
        result = createCommandPools();
        if (result != Result::Success)
        {
            shutdown();
            return result;
        }

        // Create VMA allocator
        result = createVmaAllocator();
        if (result != Result::Success)
        {
            shutdown();
            return result;
        }

        // Query capabilities
        queryCapabilities();

        initialized_ = true;
        return Result::Success;
    }

    void Device::shutdown()
    {
        if (!initialized_ && !nativeDevice_)
        {
            return;
        }

        // Wait for device idle
        if (nativeDevice_)
        {
            vkDeviceWaitIdle(nativeDevice_);
        }

        // Process all pending deletions
        for (auto& entry : deletionQueue_)
        {
            entry.deleter();
        }
        deletionQueue_.clear();

        // Destroy command pools
        if (nativeDevice_)
        {
            if (graphicsCommandPool_ && graphicsCommandPool_ != computeCommandPool_ && graphicsCommandPool_ != transferCommandPool_)
            {
                vkDestroyCommandPool(nativeDevice_, graphicsCommandPool_, nullptr);
            }
            if (computeCommandPool_ && computeCommandPool_ != graphicsCommandPool_)
            {
                vkDestroyCommandPool(nativeDevice_, computeCommandPool_, nullptr);
            }
            if (transferCommandPool_ && transferCommandPool_ != graphicsCommandPool_ && transferCommandPool_ != computeCommandPool_)
            {
                vkDestroyCommandPool(nativeDevice_, transferCommandPool_, nullptr);
            }
            graphicsCommandPool_ = nullptr;
            computeCommandPool_  = nullptr;
            transferCommandPool_ = nullptr;
        }

        // Destroy VMA allocator
        if (vmaAllocator_)
        {
            vmaDestroyAllocator(vmaAllocator_);
            vmaAllocator_ = nullptr;
        }

        // Destroy device
        if (nativeDevice_)
        {
            vkDestroyDevice(nativeDevice_, nullptr);
            nativeDevice_ = nullptr;
        }

        // Destroy debug messenger
        if (debugMessenger_ && nativeInstance_)
        {
            auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                                                                                   nativeInstance_,
                                                                                   "vkDestroyDebugUtilsMessengerEXT");
            if (func)
            {
                func(nativeInstance_, debugMessenger_, nullptr);
            }
            debugMessenger_ = nullptr;
        }

        // Destroy instance
        if (nativeInstance_)
        {
            vkDestroyInstance(nativeInstance_, nullptr);
            nativeInstance_ = nullptr;
        }

        nativePhysicalDevice_ = nullptr;
        graphicsQueue_        = nullptr;
        computeQueue_         = nullptr;
        transferQueue_        = nullptr;
        initialized_          = false;
    }

    Result Device::createInstance(bool enableValidation)
    {
        VkApplicationInfo appInfo  = {};
        appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName   = "Vulkan Engine";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName        = "Vulkan Engine RHI";
        appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion         = VK_API_VERSION_1_3;

        std::vector<const char*> extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            #ifdef VK_USE_PLATFORM_WIN32_KHR
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
            #endif
        };

        std::vector<const char*> layers;

        if (enableValidation)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            layers.push_back("VK_LAYER_KHRONOS_validation");
        }

        VkInstanceCreateInfo createInfo    = {};
        createInfo.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo        = &appInfo;
        createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();
        createInfo.enabledLayerCount       = static_cast<uint32_t>(layers.size());
        createInfo.ppEnabledLayerNames     = layers.data();

        VkResult result = vkCreateInstance(&createInfo, nullptr, &nativeInstance_);
        if (result != VK_SUCCESS)
        {
            return Result::Error_OutOfMemory;
        }

        // Create debug messenger
        if (enableValidation)
        {
            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
            debugCreateInfo.sType                              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugCreateInfo.messageSeverity                    = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugCreateInfo.pfnUserCallback = debugCallback;

            auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                                                                                  nativeInstance_,
                                                                                  "vkCreateDebugUtilsMessengerEXT");
            if (func)
            {
                func(nativeInstance_, &debugCreateInfo, nullptr, &debugMessenger_);
            }
        }

        return Result::Success;
    }

    Result Device::selectPhysicalDevice(bool preferDiscrete)
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(nativeInstance_, &deviceCount, nullptr);

        if (deviceCount == 0)
        {
            return Result::Error_Unsupported;
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(nativeInstance_, &deviceCount, devices.data());

        // Score devices
        int              bestScore  = -1;
        VkPhysicalDevice bestDevice = nullptr;

        for (const auto& device : devices)
        {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(device, &props);

            int score = 0;

            // Prefer discrete GPU
            if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                score += 1000;
            }
            else if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
            {
                score += 100;
            }

            // Check queue families
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

            bool hasGraphics = false;
            bool hasCompute  = false;
            bool hasTransfer = false;

            for (uint32_t i = 0; i < queueFamilyCount; ++i)
            {
                if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    hasGraphics = true;
                }
                if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
                {
                    hasCompute = true;
                }
                if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
                {
                    hasTransfer = true;
                }
            }

            if (!hasGraphics)
            {
                continue; // Require graphics support
            }

            // Bonus for having all queue types
            if (hasGraphics && hasCompute && hasTransfer)
            {
                score += 100;
            }

            if (score > bestScore)
            {
                bestScore  = score;
                bestDevice = device;
            }
        }

        if (!bestDevice)
        {
            return Result::Error_Unsupported;
        }

        nativePhysicalDevice_ = bestDevice;
        return Result::Success;
    }

    Result Device::createLogicalDevice()
    {
        // Find queue families
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(nativePhysicalDevice_, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(nativePhysicalDevice_, &queueFamilyCount, queueFamilies.data());

        // Find suitable queue families
        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            if (graphicsQueueFamily_ == UINT32_MAX &&
                (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
            {
                graphicsQueueFamily_ = i;
            }

            if (computeQueueFamily_ == UINT32_MAX &&
                (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
            {
                computeQueueFamily_ = i;
            }

            if (transferQueueFamily_ == UINT32_MAX &&
                (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT))
            {
                transferQueueFamily_ = i;
            }
        }

        // Use graphics family for compute and transfer if not found separately
        if (computeQueueFamily_ == UINT32_MAX)
        {
            computeQueueFamily_ = graphicsQueueFamily_;
        }
        if (transferQueueFamily_ == UINT32_MAX)
        {
            transferQueueFamily_ = graphicsQueueFamily_;
        }

        // Create queues
        std::set<uint32_t> uniqueQueueFamilies = {
            graphicsQueueFamily_,
            computeQueueFamily_,
            transferQueueFamily_
        };

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        float                                queuePriority = 1.0f;

        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex        = queueFamily;
            queueCreateInfo.queueCount              = 1;
            queueCreateInfo.pQueuePriorities        = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        // Device features
        VkPhysicalDeviceFeatures deviceFeatures = {};
        deviceFeatures.fillModeNonSolid         = VK_TRUE;
        deviceFeatures.wideLines                = VK_TRUE;

        VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures = {};
        dynamicRenderingFeatures.sType                                    = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
        dynamicRenderingFeatures.dynamicRendering                         = VK_TRUE;

        // Device extensions
        std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        };

        VkDeviceCreateInfo createInfo      = {};
        createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext                   = &dynamicRenderingFeatures;
        createInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos       = queueCreateInfos.data();
        createInfo.pEnabledFeatures        = &deviceFeatures;
        createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        VkResult result = vkCreateDevice(nativePhysicalDevice_, &createInfo, nullptr, &nativeDevice_);
        if (result != VK_SUCCESS)
        {
            return Result::Error_OutOfMemory;
        }

        // Get queues
        vkGetDeviceQueue(nativeDevice_, graphicsQueueFamily_, 0, &graphicsQueue_);
        vkGetDeviceQueue(nativeDevice_, computeQueueFamily_, 0, &computeQueue_);
        vkGetDeviceQueue(nativeDevice_, transferQueueFamily_, 0, &transferQueue_);

        return Result::Success;
    }

    Result Device::createCommandPools()
    {
        // Graphics command pool
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags                   = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex        = graphicsQueueFamily_;

        VkResult result = vkCreateCommandPool(nativeDevice_, &poolInfo, nullptr, &graphicsCommandPool_);
        if (result != VK_SUCCESS)
        {
            return Result::Error_OutOfMemory;
        }

        // Compute command pool
        if (computeQueueFamily_ != graphicsQueueFamily_)
        {
            poolInfo.queueFamilyIndex = computeQueueFamily_;
            result                    = vkCreateCommandPool(nativeDevice_, &poolInfo, nullptr, &computeCommandPool_);
            if (result != VK_SUCCESS)
            {
                return Result::Error_OutOfMemory;
            }
        }
        else
        {
            computeCommandPool_ = graphicsCommandPool_;
        }

        // Transfer command pool
        if (transferQueueFamily_ != graphicsQueueFamily_)
        {
            poolInfo.queueFamilyIndex = transferQueueFamily_;
            result                    = vkCreateCommandPool(nativeDevice_, &poolInfo, nullptr, &transferCommandPool_);
            if (result != VK_SUCCESS)
            {
                return Result::Error_OutOfMemory;
            }
        }
        else
        {
            transferCommandPool_ = graphicsCommandPool_;
        }

        return Result::Success;
    }

    Result Device::createVmaAllocator()
    {
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice         = nativePhysicalDevice_;
        allocatorInfo.device                 = nativeDevice_;
        allocatorInfo.instance               = nativeInstance_;
        allocatorInfo.vulkanApiVersion       = VK_API_VERSION_1_3;

        VkResult result = vmaCreateAllocator(&allocatorInfo, &vmaAllocator_);
        if (result != VK_SUCCESS)
        {
            return Result::Error_OutOfMemory;
        }

        return Result::Success;
    }

    void Device::queryCapabilities()
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(nativePhysicalDevice_, &props);

        capabilities_.maxImageDimension1D            = props.limits.maxImageDimension1D;
        capabilities_.maxImageDimension2D            = props.limits.maxImageDimension2D;
        capabilities_.maxImageDimension3D            = props.limits.maxImageDimension3D;
        capabilities_.maxImageDimensionCube          = props.limits.maxImageDimensionCube;
        capabilities_.maxUniformBufferRange          = props.limits.maxUniformBufferRange;
        capabilities_.maxStorageBufferRange          = props.limits.maxStorageBufferRange;
        capabilities_.maxPushConstantsSize           = props.limits.maxPushConstantsSize;
        capabilities_.maxMemoryAllocationCount       = props.limits.maxMemoryAllocationCount;
        capabilities_.maxSamplerAllocationCount      = props.limits.maxSamplerAllocationCount;
        capabilities_.maxBoundDescriptorSets         = props.limits.maxBoundDescriptorSets;
        capabilities_.maxComputeSharedMemorySize     = props.limits.maxComputeSharedMemorySize;
        capabilities_.maxComputeWorkGroupInvocations = props.limits.maxComputeWorkGroupInvocations;

        for (int i = 0; i < 3; ++i)
        {
            capabilities_.maxComputeWorkGroupCount[i] = props.limits.maxComputeWorkGroupCount[i];
            capabilities_.maxComputeWorkGroupSize[i]  = props.limits.maxComputeWorkGroupSize[i];
        }
    }

    // Resource creation implementations
    ResultValue<BufferHandle> Device::createBuffer(const BufferDesc& desc)
    {
        if (!initialized_) return std::unexpected(Result::Error_InvalidParameter);

        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size               = desc.size;
        bufferInfo.usage              = static_cast<VkBufferUsageFlags>(desc.usage);
        bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.requiredFlags           = static_cast<VkMemoryPropertyFlags>(desc.memoryProperties);

        if (desc.persistentMap)
        {
            allocInfo.flags |= VMA_ALLOCATION_CREATE_MAPPED_BIT;
        }

        VkBuffer      buffer;
        VmaAllocation allocation;
        void*         mappedData = nullptr;

        VkResult result = vmaCreateBuffer(vmaAllocator_,
                                          &bufferInfo,
                                          &allocInfo,
                                          &buffer,
                                          &allocation,
                                          nullptr);

        if (result != VK_SUCCESS)
        {
            return std::unexpected(Result::Error_OutOfMemory);
        }

        // Get mapped pointer if persistent
        if (desc.persistentMap)
        {
            vmaMapMemory(vmaAllocator_, allocation, &mappedData);
        }

        Buffer::InternalData internalData{
            .buffer = buffer,
            .allocation = allocation,
            .allocator = vmaAllocator_,
            .mappedPtr = mappedData
        };

        return std::make_shared<Buffer>(internalData, desc);
    }

    ResultValue<CommandBufferHandle> Device::allocateCommandBuffer(QueueType queueType)
    {
        if (!initialized_) return std::unexpected(Result::Error_InvalidParameter);

        VkCommandPool pool;
        switch (queueType)
        {
            case QueueType::Graphics:
                pool = graphicsCommandPool_;
                break;
            case QueueType::Compute:
                pool = computeCommandPool_;
                break;
            case QueueType::Transfer:
                pool = transferCommandPool_;
                break;
            default:
                return std::unexpected(Result::Error_InvalidParameter);
        }

        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool                 = pool;
        allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount          = 1;

        VkCommandBuffer cmd;
        VkResult        result = vkAllocateCommandBuffers(nativeDevice_, &allocInfo, &cmd);

        if (result != VK_SUCCESS)
        {
            return std::unexpected(Result::Error_OutOfMemory);
        }

        CommandBuffer::InternalData internalData{
            .cmd = cmd,
            .device = nativeDevice_,
            .pool = pool,
            .queueType = queueType
        };

        return std::make_shared<CommandBuffer>(internalData);
    }

    ResultValue<SemaphoreHandle> Device::createSemaphore()
    {
        if (!initialized_) return std::unexpected(Result::Error_InvalidParameter);

        VkSemaphoreCreateInfo createInfo = {};
        createInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkSemaphore semaphore;
        VkResult    result = vkCreateSemaphore(nativeDevice_, &createInfo, nullptr, &semaphore);

        if (result != VK_SUCCESS)
        {
            return std::unexpected(Result::Error_OutOfMemory);
        }

        Semaphore::InternalData internalData{
            .semaphore = semaphore,
            .device = nativeDevice_
        };

        return std::make_shared<Semaphore>(internalData);
    }

    ResultValue<FenceHandle> Device::createFence(bool signaled)
    {
        if (!initialized_) return std::unexpected(Result::Error_InvalidParameter);

        VkFenceCreateInfo createInfo = {};
        createInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        if (signaled)
        {
            createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        }

        VkFence  fence;
        VkResult result = vkCreateFence(nativeDevice_, &createInfo, nullptr, &fence);

        if (result != VK_SUCCESS)
        {
            return std::unexpected(Result::Error_OutOfMemory);
        }

        Fence::InternalData internalData{
            .fence = fence,
            .device = nativeDevice_
        };

        return std::make_shared<Fence>(internalData);
    }

    Result Device::submit(QueueType queueType, const SubmitInfo& info, FenceHandle fence)
    {
        if (!initialized_) return Result::Error_InvalidParameter;

        VkQueue queue;
        switch (queueType)
        {
            case QueueType::Graphics:
                queue = graphicsQueue_;
                break;
            case QueueType::Compute:
                queue = computeQueue_;
                break;
            case QueueType::Transfer:
                queue = transferQueue_;
                break;
            default:
                return Result::Error_InvalidParameter;
        }

        std::vector<VkCommandBuffer> cmdBuffers;
        cmdBuffers.reserve(info.commandBuffers.size());
        for (const auto& cmd : info.commandBuffers)
        {
            if (cmd) cmdBuffers.push_back(cmd->nativeHandle());
        }

        std::vector<VkSemaphore> waitSemaphores;
        waitSemaphores.reserve(info.waitSemaphores.size());
        for (const auto& sem : info.waitSemaphores)
        {
            if (sem) waitSemaphores.push_back(sem->nativeHandle());
        }

        std::vector<VkPipelineStageFlags> waitStages;
        waitStages.reserve(info.waitStages.size());
        for (const auto& stage : info.waitStages)
        {
            waitStages.push_back(static_cast<VkPipelineStageFlags>(stage));
        }

        std::vector<VkSemaphore> signalSemaphores;
        signalSemaphores.reserve(info.signalSemaphores.size());
        for (const auto& sem : info.signalSemaphores)
        {
            if (sem) signalSemaphores.push_back(sem->nativeHandle());
        }

        VkSubmitInfo submitInfo         = {};
        submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount   = static_cast<uint32_t>(waitSemaphores.size());
        submitInfo.pWaitSemaphores      = waitSemaphores.empty() ? nullptr : waitSemaphores.data();
        submitInfo.pWaitDstStageMask    = waitStages.empty() ? nullptr : waitStages.data();
        submitInfo.commandBufferCount   = static_cast<uint32_t>(cmdBuffers.size());
        submitInfo.pCommandBuffers      = cmdBuffers.empty() ? nullptr : cmdBuffers.data();
        submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
        submitInfo.pSignalSemaphores    = signalSemaphores.empty() ? nullptr : signalSemaphores.data();

        VkFence vkFence = fence ? fence->nativeHandle() : nullptr;

        VkResult result = vkQueueSubmit(queue, 1, &submitInfo, vkFence);

        if (result != VK_SUCCESS)
        {
            return Result::Error_DeviceLost;
        }

        return Result::Success;
    }

    Result Device::waitIdle()
    {
        if (!initialized_) return Result::Error_InvalidParameter;

        VkResult result = vkDeviceWaitIdle(nativeDevice_);
        if (result != VK_SUCCESS)
        {
            return Result::Error_DeviceLost;
        }

        return Result::Success;
    }

    void Device::runGarbageCollection()
    {
        // Delete resources that are at least MAX_FRAMES_IN_FLIGHT frames old
        const uint32_t safeFrame = currentFrameIndex_ - MAX_FRAMES_IN_FLIGHT;

        deletionQueue_.erase(
                             std::remove_if(deletionQueue_.begin(),
                                            deletionQueue_.end(),
                                            [safeFrame](const DeletionEntry& entry)
                                            {
                                                if (entry.frameIndex <= safeFrame)
                                                {
                                                    entry.deleter();
                                                    return true;
                                                }
                                                return false;
                                            }),
                             deletionQueue_.end()
                            );

        currentFrameIndex_++;
    }

    // Stub implementations for remaining functions
    ResultValue<TextureHandle> Device::createTexture(const TextureDesc& desc)
    {
        (void)desc;
        return std::unexpected(Result::Error_Unsupported);
    }

    ResultValue<TextureViewHandle> Device::createTextureView(
        TextureHandle          texture,
        const TextureViewDesc& desc)
    {
        (void)texture;
        (void)desc;
        return std::unexpected(Result::Error_Unsupported);
    }

    ResultValue<SamplerHandle> Device::createSampler(const SamplerDesc& desc)
    {
        (void)desc;
        return std::unexpected(Result::Error_Unsupported);
    }

    ResultValue<ShaderHandle> Device::createShader(const ShaderDesc& desc)
    {
        (void)desc;
        return std::unexpected(Result::Error_Unsupported);
    }

    ResultValue<DescriptorSetLayoutHandle> Device::createDescriptorSetLayout(
        const DescriptorSetLayoutDesc& desc)
    {
        (void)desc;
        return std::unexpected(Result::Error_Unsupported);
    }

    ResultValue<PipelineLayoutHandle> Device::createPipelineLayout(
        const PipelineLayoutDesc& desc)
    {
        (void)desc;
        return std::unexpected(Result::Error_Unsupported);
    }

    ResultValue<GraphicsPipelineHandle> Device::createGraphicsPipeline(
        const GraphicsPipelineDesc& desc)
    {
        (void)desc;
        return std::unexpected(Result::Error_Unsupported);
    }

    ResultValue<ComputePipelineHandle> Device::createComputePipeline(
        const ComputePipelineDesc& desc)
    {
        (void)desc;
        return std::unexpected(Result::Error_Unsupported);
    }

    ResultValue<DescriptorSetHandle> Device::allocateDescriptorSet(
        DescriptorSetLayoutHandle layout)
    {
        (void)layout;
        return std::unexpected(Result::Error_Unsupported);
    }

    ResultValue<DescriptorSetHandle> Device::allocateDescriptorSet(
        DescriptorSetLayoutHandle                              layout,
        const std::vector<DescriptorSetHandle::element_type*>& resources)
    {
        (void)layout;
        (void)resources;
        return std::unexpected(Result::Error_Unsupported);
    }

    ResultValue<SwapChainHandle> Device::createSwapChain(const SwapChainDesc& desc)
    {
        (void)desc;
        return std::unexpected(Result::Error_Unsupported);
    }

    void Device::destroySwapChain(SwapChainHandle swapChain)
    {
        (void)swapChain;
    }

    std::vector<CommandBufferHandle> Device::allocateCommandBuffers(QueueType queueType, uint32_t count)
    {
        std::vector<CommandBufferHandle> result;
        result.reserve(count);

        for (uint32_t i = 0; i < count; ++i)
        {
            auto cmd = allocateCommandBuffer(queueType);
            if (cmd)
            {
                result.push_back(cmd.value());
            }
        }

        return result;
    }

    void Device::freeCommandBuffer(CommandBufferHandle cmd)
    {
        (void)cmd;
    }

    void Device::freeCommandBuffers(const std::vector<CommandBufferHandle>& cmds)
    {
        (void)cmds;
    }

    void Device::resetCommandPool(QueueType queueType)
    {
        VkCommandPool pool;
        switch (queueType)
        {
            case QueueType::Graphics:
                pool = graphicsCommandPool_;
                break;
            case QueueType::Compute:
                pool = computeCommandPool_;
                break;
            case QueueType::Transfer:
                pool = transferCommandPool_;
                break;
            default:
                return;
        }

        vkResetCommandPool(nativeDevice_, pool, 0);
    }

    void Device::destroySemaphore(SemaphoreHandle semaphore)
    {
        if (semaphore)
        {
            deletionQueue_.push_back({
                                         currentFrameIndex_,
                                         [semaphore]() mutable { semaphore.reset(); }
                                     });
        }
    }

    void Device::destroyFence(FenceHandle fence)
    {
        if (fence)
        {
            deletionQueue_.push_back({
                                         currentFrameIndex_,
                                         [fence]() mutable { fence.reset(); }
                                     });
        }
    }

    Result Device::waitForFence(FenceHandle fence, uint64_t timeout)
    {
        if (!fence) return Result::Error_InvalidParameter;
        return fence->wait(timeout);
    }

    Result Device::resetFence(FenceHandle fence)
    {
        if (!fence) return Result::Error_InvalidParameter;
        return fence->reset();
    }

    ResultValue<bool> Device::isFenceSignaled(FenceHandle fence)
    {
        if (!fence) return std::unexpected(Result::Error_InvalidParameter);
        return fence->isSignaled();
    }

    Result Device::present(
        SwapChainHandle                     swapChain,
        const std::vector<SemaphoreHandle>& waitSemaphores)
    {
        if (!swapChain) return Result::Error_InvalidParameter;
        return swapChain->present(waitSemaphores);
    }
} // namespace engine::rhi

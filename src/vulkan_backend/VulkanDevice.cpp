#include "vulkan_backend/VulkanDevice.h"
#include <cstring>
#include <stdexcept>
#include <vector>

namespace
{
    //必须支持的功能
    bool hasAnyRequiredFeature(const VkPhysicalDeviceFeatures& required)
    {
        return required.samplerAnisotropy == VK_TRUE ||
               required.sampleRateShading == VK_TRUE ||
               required.fillModeNonSolid == VK_TRUE ||
               required.wideLines == VK_TRUE ||
               required.geometryShader == VK_TRUE ||
               required.tessellationShader == VK_TRUE;
    }

    bool checkDeviceExtensionSupport(
        VkPhysicalDevice                device,
        const std::vector<const char*>& required)
    {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> available(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, available.data());

        for (const char* req : required)
        {
            bool found = false;
            for (const auto& ext : available)
            {
                if (std::strcmp(req, ext.extensionName) == 0)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                return false;
            }
        }
        return true;
    }

    // 检查“实际支持的 features”是否覆盖“requiredFeatures 为 VK_TRUE 的字段”
    bool checkFeatureSupport(
        const VkPhysicalDeviceFeatures& supported,
        const VkPhysicalDeviceFeatures& required)
    {
        // 简单逐字段检查你关心的子集即可；不需要每个字段都列完
        auto require = [&](VkBool32 sup, VkBool32 req) -> bool
        {
            return req == VK_FALSE || (req == VK_TRUE && sup == VK_TRUE);
        };

        if (!require(supported.samplerAnisotropy, required.samplerAnisotropy)) return false;
        if (!require(supported.sampleRateShading, required.sampleRateShading)) return false;
        if (!require(supported.fillModeNonSolid, required.fillModeNonSolid)) return false;
        if (!require(supported.wideLines, required.wideLines)) return false;
        if (!require(supported.geometryShader, required.geometryShader)) return false;
        if (!require(supported.tessellationShader, required.tessellationShader))return false;

        // 后续如果有更多需要，可以继续在这里加字段检查

        return true;
    }

    bool isDeviceSuitable(
        VkPhysicalDevice          device,
        VkSurfaceKHR              surface,
        const VulkanDeviceConfig& config)
    {
        // 1. 扩展
        if (!checkDeviceExtensionSupport(device, config.requiredExtensions))
        {
            return false;
        }

        // 2. 队列：至少有 graphics + present
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        bool hasGraphics = false;
        bool hasPresent  = false;

        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            const auto& props = queueFamilies[i];

            if (props.queueCount > 0 && (props.queueFlags & VK_QUEUE_GRAPHICS_BIT))
            {
                hasGraphics = true;
            }

            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport)
            {
                hasPresent = true;
            }
        }

        if (!hasGraphics || !hasPresent)
        {
            return false;
        }

        // 3. surface 格式 / present mode
        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount == 0)
        {
            return false;
        }

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if (presentModeCount == 0)
        {
            return false;
        }

        // 4. 特性
        if (hasAnyRequiredFeature(config.requiredFeatures))
        {
            VkPhysicalDeviceFeatures supported{};
            vkGetPhysicalDeviceFeatures(device, &supported);

            if (!checkFeatureSupport(supported, config.requiredFeatures))
            {
                return false;
            }
        }

        return true;
    }
}

VulkanDevice::VulkanDevice(VkInstance instance, VkSurfaceKHR surface, const VulkanDeviceConfig& config)
    : instance_(instance)
{
    // 1. 枚举物理设备
    uint32_t deviceCount = 0;
    if (vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr) != VK_SUCCESS || deviceCount == 0)
    {
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    physicalDevice_ = VK_NULL_HANDLE;
    for (VkPhysicalDevice dev : devices)
    {
        if (isDeviceSuitable(dev, surface, config))
        {
            physicalDevice_ = dev;
            break;
        }
    }

    if (physicalDevice_ == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to find a suitable physical device");
    }

    vkGetPhysicalDeviceProperties(physicalDevice_, &properties_);        // 获取物理设备属性
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProps_); // 获取物理设备内存属性

    // 2. 确定 graphics / present 队列族 index
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyCount, nullptr);// 获取队列族数量
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyCount, queueFamilies.data());// 获取队列族属性

    std::optional<uint32_t> graphicsFamily;// 查找支持图形的队列族
    std::optional<uint32_t> presentFamily;// 查找支持图形和呈现的队列族

    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        const auto& props = queueFamilies[i];

        if (props.queueCount > 0 && (props.queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            if (!graphicsFamily) graphicsFamily = i;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice_, i, surface, &presentSupport);
        if (presentSupport && !presentFamily)
        {
            presentFamily = i;
        }
    }

    if (!graphicsFamily)
    {
        throw std::runtime_error("Failed to find graphics queue family");
    }
    if (!presentFamily)
    {
        presentFamily = graphicsFamily;
    }

    graphicsQueue_.familyIndex = graphicsFamily.value();
    presentQueue_.familyIndex  = presentFamily.value();

    // 3. 创建设备和队列
    float                                queuePriority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    VkDeviceQueueCreateInfo graphicsQueueInfo{};
    graphicsQueueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphicsQueueInfo.queueFamilyIndex = graphicsQueue_.familyIndex;
    graphicsQueueInfo.queueCount       = 1;
    graphicsQueueInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(graphicsQueueInfo);

    if (presentQueue_.familyIndex != graphicsQueue_.familyIndex)
    {
        VkDeviceQueueCreateInfo presentQueueInfo{};
        presentQueueInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        presentQueueInfo.queueFamilyIndex = presentQueue_.familyIndex;
        presentQueueInfo.queueCount       = 1;
        presentQueueInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(presentQueueInfo);
    }

    VkPhysicalDeviceFeatures enabledFeatures{};
    // 只启用你要求为 VK_TRUE 的特性（避免开启未支持的）
    enabledFeatures = config.requiredFeatures;

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
    deviceInfo.pQueueCreateInfos       = queueCreateInfos.data();
    deviceInfo.enabledExtensionCount   = static_cast<uint32_t>(config.requiredExtensions.size());
    deviceInfo.ppEnabledExtensionNames = config.requiredExtensions.data();
    deviceInfo.pEnabledFeatures        = &enabledFeatures;

    if (vkCreateDevice(physicalDevice_, &deviceInfo, nullptr, &device_) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create logical device");
    }

    vkGetDeviceQueue(device_, graphicsQueue_.familyIndex, 0, &graphicsQueue_.handle);
    vkGetDeviceQueue(device_, presentQueue_.familyIndex, 0, &presentQueue_.handle);
}


VulkanDevice::~VulkanDevice()
{
    // 确保设备上的所有操作完成后再销毁设备
    if (device_ != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(device_);
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }

    instance_       = VK_NULL_HANDLE;
    physicalDevice_ = VK_NULL_HANDLE;
}

std::optional<VulkanDevice::QueueInfo> VulkanDevice::computeQueue() const noexcept
{
    return computeQueue_;
}

std::optional<VulkanDevice::QueueInfo> VulkanDevice::transferQueue() const noexcept
{
    return transferQueue_;
}

const VkPhysicalDeviceProperties& VulkanDevice::properties() const noexcept
{
    return properties_;
}

const VkPhysicalDeviceMemoryProperties& VulkanDevice::memoryProperties() const noexcept
{
    return memoryProps_;
}

bool VulkanDevice::supportsPresentation(VkSurfaceKHR surface, uint32_t family) const
{
    VkBool32 presentSupport = VK_FALSE;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice_, family, surface, &presentSupport);
    return presentSupport == VK_TRUE;
}

bool VulkanDevice::supportsFormat(VkFormat fmt, VkImageTiling tiling, VkFormatFeatureFlags features) const
{
    VkFormatProperties formatProps{};
    vkGetPhysicalDeviceFormatProperties(physicalDevice_, fmt, &formatProps);

    if (tiling == VK_IMAGE_TILING_LINEAR)
    {
        return (formatProps.linearTilingFeatures & features) == features;
    }
    if (tiling == VK_IMAGE_TILING_OPTIMAL)
    {
        return (formatProps.optimalTilingFeatures & features) == features;
    }
    return false;
}

VkCommandPool VulkanDevice::createCommandPool(uint32_t familyIndex, VkCommandPoolCreateFlags flags) const
{
    VkCommandPoolCreateInfo info{};
    info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    info.flags            = flags;
    info.queueFamilyIndex = familyIndex;

    VkCommandPool pool = VK_NULL_HANDLE;
    if (vkCreateCommandPool(device_, &info, nullptr, &pool) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command pool");
    }
    return pool;
}

void VulkanDevice::submitImmediate(uint32_t queueFamily, std::function<void(VkCommandBuffer)> recordFn) const
{
    // 创建临时命令池
    VkCommandPool pool = createCommandPool(queueFamily, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = pool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer cmd = VK_NULL_HANDLE;
    vkAllocateCommandBuffers(device_, &allocInfo, &cmd);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(cmd, &beginInfo);
    recordFn(cmd);
    vkEndCommandBuffer(cmd);

    // 简化：根据 familyIndex 选择队列，目前只支持 graphics/present
    VkQueue submitQueue = graphicsQueue_.handle;
    if (queueFamily == presentQueue_.familyIndex)
    {
        submitQueue = presentQueue_.handle;
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &cmd;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    VkFence fence = VK_NULL_HANDLE;
    vkCreateFence(device_, &fenceInfo, nullptr, &fence);

    vkQueueSubmit(submitQueue, 1, &submitInfo, fence);
    vkWaitForFences(device_, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(device_, fence, nullptr);
    vkFreeCommandBuffers(device_, pool, 1, &cmd);
    vkDestroyCommandPool(device_, pool, nullptr);
}

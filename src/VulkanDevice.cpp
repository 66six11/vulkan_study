#include "VulkanDevice.h"
#include <cstring>
#include <stdexcept>
#include <vector>

namespace {

std::vector<const char*> toVkExtensionNames(const std::vector<DeviceExtension>& exts)
{
    std::vector<const char*> names;
    names.reserve(exts.size());

    for (DeviceExtension e : exts)
    {
        switch (e)
        {
            case DeviceExtension::Swapchain:
                names.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
                break;
        }
    }
    return names;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice device, const std::vector<const char*>& required)
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

void applyRequiredFeatures(const std::vector<DeviceFeature>& required,
                           VkPhysicalDeviceFeatures&         enabled,
                           const VkPhysicalDeviceFeatures&   supported)
{
    enabled = {};

    auto require = [&](VkBool32 supportedFlag, VkBool32& enabledFlag) {
        if (!supportedFlag)
        {
            throw std::runtime_error("Required device feature not supported");
        }
        enabledFlag = VK_TRUE;
    };

    for (DeviceFeature f : required)
    {
        switch (f)
        {
            case DeviceFeature::SamplerAnisotropy:
                require(supported.samplerAnisotropy, enabled.samplerAnisotropy);
                break;
            case DeviceFeature::SampleRateShading:
                require(supported.sampleRateShading, enabled.sampleRateShading);
                break;
            case DeviceFeature::FillModeNonSolid:
                require(supported.fillModeNonSolid, enabled.fillModeNonSolid);
                break;
            case DeviceFeature::WideLines:
                require(supported.wideLines, enabled.wideLines);
                break;
            case DeviceFeature::GeometryShader:
                require(supported.geometryShader, enabled.geometryShader);
                break;
            case DeviceFeature::TessellationShader:
                require(supported.tessellationShader, enabled.tessellationShader);
                break;
        }
    }
}

struct QueueFamilySupport
{
    bool hasGraphics = false;
    bool hasPresent  = false;
    bool hasCompute  = false;
    bool hasTransfer = false;
};

QueueFamilySupport queryQueueSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    QueueFamilySupport result{};

    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> props(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, props.data());

    for (uint32_t i = 0; i < count; ++i)
    {
        const auto& q = props[i];

        if (q.queueCount > 0 && (q.queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            result.hasGraphics = true;
        }
        if (q.queueCount > 0 && (q.queueFlags & VK_QUEUE_COMPUTE_BIT))
        {
            result.hasCompute = true;
        }
        if (q.queueCount > 0 && (q.queueFlags & VK_QUEUE_TRANSFER_BIT))
        {
            result.hasTransfer = true;
        }

        VkBool32 presentSupport = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport)
        {
            result.hasPresent = true;
        }
    }

    return result;
}

bool checkQueueRequirements(const QueueFamilySupport&           support,
                            const std::vector<QueueCapability>& required)
{
    for (QueueCapability q : required)
    {
        switch (q)
        {
            case QueueCapability::Graphics:
                if (!support.hasGraphics) return false;
                break;
            case QueueCapability::Present:
                if (!support.hasPresent) return false;
                break;
            case QueueCapability::Compute:
                if (!support.hasCompute) return false;
                break;
            case QueueCapability::Transfer:
                if (!support.hasTransfer) return false;
                break;
        }
    }
    return true;
}

bool isDeviceSuitable(VkPhysicalDevice          device,
                      VkSurfaceKHR              surface,
                      const VulkanDeviceConfig& config,
                      VkPhysicalDeviceFeatures& enabledFeaturesOut)
{
    auto requiredExtNames = toVkExtensionNames(config.requiredExtensions);
    if (!checkDeviceExtensionSupport(device, requiredExtNames))
    {
        return false;
    }

    QueueFamilySupport qSupport = queryQueueSupport(device, surface);
    if (!checkQueueRequirements(qSupport, config.requiredQueues))
    {
        return false;
    }

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

    VkPhysicalDeviceFeatures supported{};
    vkGetPhysicalDeviceFeatures(device, &supported);

    VkPhysicalDeviceFeatures enabled{};
    applyRequiredFeatures(config.requiredFeatures, enabled, supported);

    enabledFeaturesOut = enabled;
    return true;
}

} // namespace

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

    // 简化：选择第一个满足基本要求的物理设备
    physicalDevice_ = VK_NULL_HANDLE;
    for (VkPhysicalDevice dev : devices)
    {
        // 这里可以增加更多筛选逻辑，例如检查扩展、特性等
        physicalDevice_ = dev;
        break;
    }

    if (physicalDevice_ == VK_NULL_HANDLE)
    {
        throw std::runtime_error("Failed to select a suitable physical device");
    }

    vkGetPhysicalDeviceProperties(physicalDevice_, &properties_);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memoryProps_);

    // 2. 查询队列族
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &queueFamilyCount, queueFamilies.data());

    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    for (uint32_t i = 0; i < queueFamilyCount; ++i)
    {
        const auto& props = queueFamilies[i];

        if (props.queueCount > 0 && (props.queueFlags & VK_QUEUE_GRAPHICS_BIT))
        {
            if (!graphicsFamily)
            {
                graphicsFamily = i;
            }
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
        // 如果没有专门的 present 队列，就退回到 graphics 队列
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

    // 必要扩展：目前至少需要 swapchain 扩展
    const std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkPhysicalDeviceFeatures deviceFeatures{}; // 按需要开启特性

    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount    = static_cast<uint32_t>(queueCreateInfos.size());
    deviceInfo.pQueueCreateInfos       = queueCreateInfos.data();
    deviceInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
    deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
    deviceInfo.pEnabledFeatures        = &deviceFeatures;

    if (vkCreateDevice(physicalDevice_, &deviceInfo, nullptr, &device_) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create logical device");
    }

    vkGetDeviceQueue(device_, graphicsQueue_.familyIndex, 0, &graphicsQueue_.handle);
    vkGetDeviceQueue(device_, presentQueue_.familyIndex, 0, &presentQueue_.handle);
}

VulkanDevice::~VulkanDevice()
{
    if (device_ != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(device_);
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }
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

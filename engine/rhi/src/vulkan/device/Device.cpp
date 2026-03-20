#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include "vulkan/device/Device.hpp"
#include "core/utils/Logger.hpp"
#include <set>
#include <string>
#include <vector>

namespace vulkan_engine::vulkan
{
    DeviceManager::DeviceManager(const CreateInfo& create_info)
        : create_info_(create_info)
    {
    }

    DeviceManager::~DeviceManager()
    {
        shutdown();
    }

    bool DeviceManager::initialize()
    {
        if (!create_instance())
        {
            LOG_ERROR("Failed to create Vulkan instance");
            return false;
        }

        if (!select_physical_device())
        {
            LOG_ERROR("Failed to select physical device");
            return false;
        }

        if (!create_logical_device())
        {
            LOG_ERROR("Failed to create logical device");
            return false;
        }

        if (create_info_.enable_validation && create_info_.enable_debug_utils)
        {
            setup_debug_messenger();
        }

        LOG_INFO("DeviceManager initialized successfully");
        return true;
    }

    void DeviceManager::shutdown()
    {
        if (create_info_.enable_validation && create_info_.enable_debug_utils)
        {
            auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                 instance_,
                 "vkDestroyDebugUtilsMessengerEXT");
            if (vkDestroyDebugUtilsMessengerEXT && debug_messenger_ != VK_NULL_HANDLE)
            {
                vkDestroyDebugUtilsMessengerEXT(instance_, debug_messenger_, nullptr);
            }
        }

        if (device_)
        {
            vkDestroyDevice(device_, nullptr);
            device_ = VK_NULL_HANDLE;
        }

        if (instance_)
        {
            vkDestroyInstance(instance_, nullptr);
            instance_ = VK_NULL_HANDLE;
        }
    }

    bool DeviceManager::supports_feature(const DeviceFeatures& /*required*/) const
    {
        // Check if all required features are supported
        return true; // Placeholder
    }

    uint32_t DeviceManager::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) const
    {
        for (uint32_t i = 0; i < memory_properties_.memoryTypeCount; i++)
        {
            if ((type_filter & (1 << i)) && (memory_properties_.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable memory type");
    }

    bool DeviceManager::create_instance()
    {
        VkApplicationInfo app_info{};
        app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName   = create_info_.application_name.c_str();
        app_info.applicationVersion = create_info_.application_version;
        app_info.pEngineName        = create_info_.engine_name.c_str();
        app_info.engineVersion      = create_info_.engine_version;
        app_info.apiVersion         = VK_API_VERSION_1_3;

        VkInstanceCreateInfo create_info{};
        create_info.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo = &app_info;

        // Required extensions
        std::vector<const char*> extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            #ifdef _WIN32
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
            #endif
        };

        if (create_info_.enable_debug_utils)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        create_info.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
        create_info.ppEnabledExtensionNames = extensions.data();

        // Validation layers
        std::vector<const char*> validation_layers;
        if (create_info_.enable_validation)
        {
            validation_layers.push_back("VK_LAYER_KHRONOS_validation");
        }

        create_info.enabledLayerCount   = static_cast<uint32_t>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();

        VkInstance instance_handle = VK_NULL_HANDLE;
        VkResult   result          = vkCreateInstance(&create_info, nullptr, &instance_handle);
        if (result == VK_SUCCESS)
        {
            instance_.set_handle(instance_handle);
        }
        return result == VK_SUCCESS;
    }

    bool DeviceManager::select_physical_device()
    {
        uint32_t device_count = 0;
        vkEnumeratePhysicalDevices(instance_.handle(), &device_count, nullptr);

        if (device_count == 0)
        {
            return false;
        }

        std::vector<VkPhysicalDevice> devices(device_count);
        vkEnumeratePhysicalDevices(instance_.handle(), &device_count, devices.data());

        // Score and select best device
        uint64_t         best_score  = 0;
        VkPhysicalDevice best_device = VK_NULL_HANDLE;

        for (auto device : devices)
        {
            uint64_t score = static_cast<uint64_t>(score_device(device));
            if (score > best_score)
            {
                best_score  = score;
                best_device = device;
            }
        }

        if (best_device == VK_NULL_HANDLE)
        {
            return false;
        }

        physical_device_ = PhysicalDevice(best_device);
        vkGetPhysicalDeviceMemoryProperties(physical_device_, &memory_properties_);
        vkGetPhysicalDeviceProperties(physical_device_, &properties_);

        return true;
    }

    bool DeviceManager::create_logical_device()
    {
        // Find queue families
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, nullptr);

        std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &queue_family_count, queue_families.data());

        // Find graphics queue family
        for (uint32_t i = 0; i < queue_family_count; i++)
        {
            if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                graphics_family_.index       = i;
                graphics_family_.flags       = queue_families[i].queueFlags;
                graphics_family_.queue_count = queue_families[i].queueCount;
                break;
            }
        }

        if (!graphics_family_.valid())
        {
            return false;
        }

        // Device features
        VkPhysicalDeviceFeatures device_features{};

        // Create device queues
        std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
        std::set<uint32_t>                   unique_queue_families = {graphics_family_.index};

        float queue_priority = 1.0f;
        for (uint32_t queue_family : unique_queue_families)
        {
            VkDeviceQueueCreateInfo queue_create_info{};
            queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queue_create_info.queueFamilyIndex = queue_family;
            queue_create_info.queueCount       = 1;
            queue_create_info.pQueuePriorities = &queue_priority;
            queue_create_infos.push_back(queue_create_info);
        }

        // Required device extensions
        std::vector<const char*> device_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME // Enable Dynamic Rendering
        };

        // Enable Dynamic Rendering feature
        VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features{};
        dynamic_rendering_features.sType            = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
        dynamic_rendering_features.dynamicRendering = VK_TRUE;

        VkDeviceCreateInfo create_info{};
        create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.pNext                   = &dynamic_rendering_features; // Chain dynamic rendering features
        create_info.queueCreateInfoCount    = static_cast<uint32_t>(queue_create_infos.size());
        create_info.pQueueCreateInfos       = queue_create_infos.data();
        create_info.pEnabledFeatures        = &device_features;
        create_info.enabledExtensionCount   = static_cast<uint32_t>(device_extensions.size());
        create_info.ppEnabledExtensionNames = device_extensions.data();

        VkDevice device_handle = VK_NULL_HANDLE;
        VkResult result        = vkCreateDevice(physical_device_.handle(), &create_info, nullptr, &device_handle);
        if (result != VK_SUCCESS)
        {
            return false;
        }
        device_.set_handle(device_handle);

        // Mark dynamic rendering as enabled
        features_.dynamic_rendering = true;
        LOG_INFO("Dynamic Rendering enabled");

        // Get graphics queue
        VkQueue queue_handle = VK_NULL_HANDLE;
        vkGetDeviceQueue(device_.handle(), graphics_family_.index, 0, &queue_handle);
        graphics_queue_.set_handle(queue_handle);

        return true;
    }

    bool DeviceManager::setup_queues()
    {
        // Already done in create_logical_device
        return true;
    }

    uint64_t DeviceManager::score_device(VkPhysicalDevice device) const
    {
        VkPhysicalDeviceProperties properties;
        VkPhysicalDeviceFeatures   features;
        vkGetPhysicalDeviceProperties(device, &properties);
        vkGetPhysicalDeviceFeatures(device, &features);

        int64_t score = 0;

        // Prefer discrete GPU
        if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            score += 1000;
        }

        // Score based on VRAM
        VkPhysicalDeviceMemoryProperties mem_properties;
        vkGetPhysicalDeviceMemoryProperties(device, &mem_properties);

        uint64_t vram_size = 0;
        for (uint32_t i = 0; i < mem_properties.memoryHeapCount; i++)
        {
            if (mem_properties.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT)
            {
                vram_size += mem_properties.memoryHeaps[i].size;
            }
        }
        score += static_cast<int64_t>(vram_size / (1024 * 1024 * 1024)); // Add points per GB

        return score;
    }

    bool DeviceManager::check_device_support(VkPhysicalDevice device) const
    {
        // Check if device supports required extensions
        uint32_t extension_count;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

        std::vector<VkExtensionProperties> available_extensions(extension_count);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions.data());

        std::set<std::string> required_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
        };

        for (const auto& extension : available_extensions)
        {
            required_extensions.erase(extension.extensionName);
        }

        // Also check if dynamic rendering feature is supported
        VkPhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features{};
        dynamic_rendering_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;

        VkPhysicalDeviceFeatures2 features2{};
        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        features2.pNext = &dynamic_rendering_features;

        vkGetPhysicalDeviceFeatures2(device, &features2);

        if (!dynamic_rendering_features.dynamicRendering)
        {
            LOG_WARN("Device does not support Dynamic Rendering");
            return false;
        }

        return required_extensions.empty();
    }

    bool DeviceManager::setup_debug_messenger()
    {
        VkDebugUtilsMessengerCreateInfoEXT create_info{};
        create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                  VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        create_info.pfnUserCallback = debug_callback;

        auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
             instance_,
             "vkCreateDebugUtilsMessengerEXT");

        if (!vkCreateDebugUtilsMessengerEXT)
        {
            return false;
        }

        return vkCreateDebugUtilsMessengerEXT(instance_, &create_info, nullptr, &debug_messenger_) == VK_SUCCESS;
    }

    VKAPI_ATTR VkBool32 VKAPI_CALL DeviceManager::debug_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
        VkDebugUtilsMessageTypeFlagsEXT /*message_type*/,
        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
        void* /*user_data*/)
    {
        if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        {
            LOG_ERROR("Vulkan Validation: " << callback_data->pMessage);
        }
        else if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        {
            LOG_WARN("Vulkan Validation: " << callback_data->pMessage);
        }
        return VK_FALSE;
    }
} // namespace vulkan_engine::vulkan
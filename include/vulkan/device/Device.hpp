#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <string>
#include <concepts>
#include <optional>

namespace vulkan_engine::vulkan
{
    // Type-safe Vulkan handle wrappers
    template <typename Tag, typename HandleType> class VulkanHandleBase
    {
        public:
            using Handle = HandleType;

            constexpr VulkanHandleBase() noexcept : handle_{VK_NULL_HANDLE}
            {
            }

            constexpr explicit VulkanHandleBase(HandleType handle) noexcept : handle_{handle}
            {
            }

            constexpr bool valid() const noexcept { return handle_ != VK_NULL_HANDLE; }
            constexpr      operator HandleType() const noexcept { return handle_; }

            constexpr bool operator==(const VulkanHandleBase& other) const noexcept
            {
                return handle_ == other.handle_;
            }

            constexpr bool operator!=(const VulkanHandleBase& other) const noexcept
            {
                return handle_ != other.handle_;
            }

            // Accessor for raw Vulkan handle
            constexpr HandleType handle() const noexcept { return handle_; }

            // Setter for initialization (used by DeviceManager)
            void set_handle(HandleType handle) noexcept { handle_ = handle; }

            // Assignment from nullptr
            VulkanHandleBase& operator=(std::nullptr_t) noexcept
            {
                handle_ = VK_NULL_HANDLE;
                return *this;
            }

        protected:
            HandleType handle_;
    };

    // Specific Vulkan handle types

    struct InstanceTag

    {
    };

    struct DeviceTag

    {
    };

    struct PhysicalDeviceTag

    {
    };

    struct QueueTag

    {
    };

    struct CommandPoolTag

    {
    };

    struct CommandBufferTag

    {
    };

    using Instance = VulkanHandleBase<InstanceTag, VkInstance>;

    using Device = VulkanHandleBase<DeviceTag, VkDevice>;

    using PhysicalDevice = VulkanHandleBase<PhysicalDeviceTag, VkPhysicalDevice>;

    using Queue = VulkanHandleBase<QueueTag, VkQueue>;

    using CommandPool = VulkanHandleBase<CommandPoolTag, VkCommandPool>;

    using CommandBuffer = VulkanHandleBase<CommandBufferTag, VkCommandBuffer>;

    // Device capabilities
    struct DeviceFeatures
    {
        bool dynamic_rendering     : 1 = false;
        bool bindless_textures     : 1 = false;
        bool mesh_shading          : 1 = false;
        bool ray_tracing           : 1 = false;
        bool multiview             : 1 = false;
        bool timeline_semaphores   : 1 = false;
        bool buffer_device_address : 1 = false;
        bool descriptor_indexing   : 1 = false;
    };

    // Queue family information
    struct QueueFamily
    {
        uint32_t     index       = UINT32_MAX;
        VkQueueFlags flags       = 0;
        uint32_t     queue_count = 0;
        float        priority    = 1.0f;

        constexpr bool valid() const noexcept { return index != UINT32_MAX; }
    };

    // Device selection criteria
    struct DeviceSelectionCriteria
    {
        enum class Type
        {
            DiscreteGPU,
            IntegratedGPU,
            VirtualGPU,
            CPU
        } preferred_type = Type::DiscreteGPU;

        // Required features
        DeviceFeatures required_features{};

        // Required extensions
        std::vector<const char*> required_extensions{};

        // Performance scoring
        uint64_t vram_size_weight    = 10;
        uint64_t compute_perf_weight = 5;
        uint64_t feature_set_weight  = 3;
    };

    // Vulkan device abstraction
    class DeviceManager
    {
        public:
            struct CreateInfo
            {
                std::string application_name    = "Vulkan Engine";
                uint32_t    application_version = VK_MAKE_VERSION(2, 0, 0);
                std::string engine_name         = "Vulkan Engine";
                uint32_t    engine_version      = VK_MAKE_VERSION(2, 0, 0);

                bool enable_validation  = true;
                bool enable_debug_utils = true;

                DeviceSelectionCriteria device_criteria{};
            };

            explicit DeviceManager(const CreateInfo& create_info);
            ~DeviceManager();

            // Core device operations
            bool initialize();
            void shutdown();

            // Device access
            Instance       instance() const { return instance_; }
            PhysicalDevice physical_device() const { return physical_device_; }
            Device         device() const { return device_; }

            // Queue access
            Queue graphics_queue() const { return graphics_queue_; }
            Queue compute_queue() const { return compute_queue_; }
            Queue transfer_queue() const { return transfer_queue_; }

            // Feature support
            const DeviceFeatures& features() const { return features_; }
            bool                  supports_feature(const DeviceFeatures& required) const;

            // Memory properties
            const VkPhysicalDeviceMemoryProperties& memory_properties() const { return memory_properties_; }

            // Utility functions
            uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties) const;

        private:
            CreateInfo create_info_;

            // Vulkan objects
            Instance       instance_;
            PhysicalDevice physical_device_;
            Device         device_;

            // Queues
            Queue graphics_queue_;
            Queue compute_queue_;
            Queue transfer_queue_;

            // Device info
            DeviceFeatures                   features_{};
            VkPhysicalDeviceProperties       properties_{};
            VkPhysicalDeviceMemoryProperties memory_properties_{};

            // Queue families
            QueueFamily graphics_family_{};
            QueueFamily compute_family_{};
            QueueFamily transfer_family_{};

            // Initialization methods
            bool create_instance();
            bool select_physical_device();
            bool create_logical_device();
            bool setup_queues();

            // Device scoring
            uint64_t score_device(VkPhysicalDevice device) const;
            bool     check_device_support(VkPhysicalDevice device) const;

            // Debug utils
            VkDebugUtilsMessengerEXT              debug_messenger_ = VK_NULL_HANDLE;
            bool                                  setup_debug_messenger();
            static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
                VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
                VkDebugUtilsMessageTypeFlagsEXT             message_type,
                const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                void*                                       user_data);
    };

    // Command buffer management
    class CommandBufferManager
    {
        public:
            explicit CommandBufferManager(std::shared_ptr<DeviceManager> device_manager);
            ~CommandBufferManager();

            // Command pool creation
            CommandPool create_command_pool(QueueFamily queue_family, VkCommandPoolCreateFlags flags = 0);

            // Command buffer allocation
            std::vector<CommandBuffer> allocate_command_buffers(
                CommandPool          pool,
                uint32_t             count,
                VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
            CommandBuffer allocate_command_buffer(CommandPool pool, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

            // Command buffer operations
            void begin_command_buffer(CommandBuffer cmd, VkCommandBufferUsageFlags flags = 0);
            void end_command_buffer(CommandBuffer cmd);
            void submit_command_buffer(Queue queue, CommandBuffer cmd, VkFence fence = VK_NULL_HANDLE);

        private:
            std::shared_ptr<DeviceManager> device_manager_;
    };

    // Device concept for generic operations
    template <typename T>
    concept VulkanDevice = requires(T device)
    {
        { device.instance() } -> std::same_as<Instance>;
        { device.physical_device() } -> std::same_as<PhysicalDevice>;
        { device.device() } -> std::same_as<Device>;
        { device.graphics_queue() } -> std::same_as<Queue>;
        { device.features() } -> std::same_as<const DeviceFeatures&>;
    };
} // namespace vulkan_engine::vulkan
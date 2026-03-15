#pragma once

#include "vulkan/device/Device.hpp"
#include <vulkan/vulkan.h>
#include <cstdint>
#include <memory>

namespace vulkan_engine::vulkan
{
    struct ImageSubresourceRange
    {
        VkImageAspectFlags aspect_mask      = VK_IMAGE_ASPECT_COLOR_BIT;
        uint32_t           base_mip_level   = 0;
        uint32_t           level_count      = 1;
        uint32_t           base_array_layer = 0;
        uint32_t           layer_count      = 1;
    };

    class Image
    {
        public:
            Image(
                std::shared_ptr<DeviceManager> device,
                uint32_t                       width,
                uint32_t                       height,
                VkFormat                       format,
                VkImageUsageFlags              usage,
                VkMemoryPropertyFlags          properties,
                uint32_t                       mip_levels   = 1,
                uint32_t                       array_layers = 1);
            ~Image();

            // Non-copyable
            Image(const Image&)            = delete;
            Image& operator=(const Image&) = delete;

            // Views
            void create_view(VkImageViewType view_type, VkFormat format, const ImageSubresourceRange& range);

            // Layout transitions
            // Set layout tracking only (no barrier emitted)
            void set_layout(VkImageLayout new_layout) { current_layout_ = new_layout; }

            // Transition layout with actual barrier emission (requires command buffer)
            void transition_layout(VkCommandBuffer cmd, VkImageLayout new_layout);

            // Get current tracked layout
            VkImageLayout current_layout() const { return current_layout_; }

            // Helper: determine access masks and stage masks for layout transition
            struct TransitionInfo
            {
                VkAccessFlags        src_access_mask;
                VkAccessFlags        dst_access_mask;
                VkPipelineStageFlags src_stage_mask;
                VkPipelineStageFlags dst_stage_mask;
            };
            static TransitionInfo get_transition_info(VkImageLayout old_layout, VkImageLayout new_layout);

            // Mipmaps
            void generate_mipmaps();

            // Data transfer
            void upload_data(const void* data, VkDeviceSize size);
            void download_data(void* data, VkDeviceSize size);

            // Accessors
            VkImage        handle() const { return image_; }
            VkImageView    view() const { return view_; }
            VkDeviceMemory memory() const { return memory_; }
            VkFormat       format() const { return format_; }
            uint32_t       width() const { return width_; }
            uint32_t       height() const { return height_; }
            uint32_t       mip_levels() const { return mip_levels_; }
            uint32_t       array_layers() const { return array_layers_; }

        private:
            std::shared_ptr<DeviceManager> device_;
            VkImage                        image_  = VK_NULL_HANDLE;
            VkImageView                    view_   = VK_NULL_HANDLE;
            VkDeviceMemory                 memory_ = VK_NULL_HANDLE;
            VkFormat                       format_;
            uint32_t                       width_          = 0;
            uint32_t                       height_         = 0;
            uint32_t                       mip_levels_     = 1;
            uint32_t                       array_layers_   = 1;
            VkImageLayout                  current_layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    class ImageBuilder
    {
        public:
            explicit ImageBuilder(std::shared_ptr<DeviceManager> device);

            ImageBuilder& width(uint32_t width);
            ImageBuilder& height(uint32_t height);
            ImageBuilder& depth(uint32_t depth);
            ImageBuilder& format(VkFormat format);
            ImageBuilder& usage(VkImageUsageFlags usage);
            ImageBuilder& mip_levels(uint32_t levels);
            ImageBuilder& array_layers(uint32_t layers);
            ImageBuilder& samples(VkSampleCountFlagBits samples);
            ImageBuilder& device_local(bool local);

            std::unique_ptr<Image> build();

        private:
            std::shared_ptr<DeviceManager> device_;
            uint32_t                       width_        = 1;
            uint32_t                       height_       = 1;
            uint32_t                       depth_        = 1;
            VkFormat                       format_       = VK_FORMAT_R8G8B8A8_UNORM;
            VkImageUsageFlags              usage_        = 0;
            uint32_t                       mip_levels_   = 1;
            uint32_t                       array_layers_ = 1;
            VkSampleCountFlagBits          samples_      = VK_SAMPLE_COUNT_1_BIT;
            VkMemoryPropertyFlags          properties_   = 0;
    };

    class ImageManager
    {
        public:
            struct Stats
            {
                uint64_t total_allocated = 0;
                uint64_t total_used      = 0;
                uint32_t image_count     = 0;
            };

            explicit ImageManager(std::shared_ptr<DeviceManager> device);
            ~ImageManager();

            std::shared_ptr<Image> create_image(
                uint32_t              width,
                uint32_t              height,
                VkFormat              format,
                VkImageUsageFlags     usage,
                VkMemoryPropertyFlags properties);
            std::shared_ptr<Image> create_color_attachment(uint32_t width, uint32_t height, VkFormat format);
            std::shared_ptr<Image> create_depth_attachment(uint32_t width, uint32_t height, VkFormat format);
            std::shared_ptr<Image> create_texture(
                uint32_t width,
                uint32_t height,
                VkFormat format,
                uint32_t mip_levels = 1);

            void destroy_image(std::shared_ptr<Image> image);

            Stats get_stats() const;

        private:
            std::shared_ptr<DeviceManager> device_;
    };
} // namespace vulkan_engine::vulkan
#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.h>
#include "engine/rhi/vulkan/device/Device.hpp"

namespace vulkan_engine::vulkan
{
    // Framebuffer attachment description
    struct FramebufferAttachment
    {
        VkImageView image_view = VK_NULL_HANDLE;
        VkFormat    format     = VK_FORMAT_UNDEFINED;
        uint32_t    layer      = 0;
    };

    // Framebuffer configuration
    struct FramebufferConfig
    {
        VkRenderPass                       render_pass = VK_NULL_HANDLE;
        uint32_t                           width       = 0;
        uint32_t                           height      = 0;
        uint32_t                           layers      = 1;
        std::vector<FramebufferAttachment> attachments;
    };

    // Framebuffer wrapper with RAII
    class Framebuffer
    {
        public:
            // Create framebuffer from config
            Framebuffer(
                std::shared_ptr<DeviceManager> device,
                const FramebufferConfig&       config);

            // Create framebuffer with explicit parameters
            Framebuffer(
                std::shared_ptr<DeviceManager>  device,
                VkRenderPass                    render_pass,
                const std::vector<VkImageView>& attachments,
                uint32_t                        width,
                uint32_t                        height,
                uint32_t                        layers = 1);

            ~Framebuffer();

            // Non-copyable
            Framebuffer(const Framebuffer&)            = delete;
            Framebuffer& operator=(const Framebuffer&) = delete;

            // Movable
            Framebuffer(Framebuffer&& other) noexcept;
            Framebuffer& operator=(Framebuffer&& other) noexcept;

            // Accessors
            VkFramebuffer handle() const { return framebuffer_; }
            bool          valid() const { return framebuffer_ != VK_NULL_HANDLE; }

            uint32_t width() const { return width_; }
            uint32_t height() const { return height_; }
            uint32_t layers() const { return layers_; }

            VkRenderPass render_pass() const { return render_pass_; }

        private:
            void create_framebuffer(
                VkRenderPass                    render_pass,
                const std::vector<VkImageView>& attachments);

            std::shared_ptr<DeviceManager> device_;
            VkFramebuffer                  framebuffer_ = VK_NULL_HANDLE;
            VkRenderPass                   render_pass_ = VK_NULL_HANDLE;
            uint32_t                       width_       = 0;
            uint32_t                       height_      = 0;
            uint32_t                       layers_      = 1;
    };

    // Framebuffer builder for convenient construction
    class FramebufferBuilder
    {
        public:
            explicit FramebufferBuilder(std::shared_ptr<DeviceManager> device);

            FramebufferBuilder& render_pass(VkRenderPass render_pass);
            FramebufferBuilder& attachment(VkImageView image_view);
            FramebufferBuilder& attachment(VkImageView image_view, VkFormat format);
            FramebufferBuilder& dimensions(uint32_t width, uint32_t height);
            FramebufferBuilder& layers(uint32_t layers);

            std::unique_ptr<Framebuffer> build();

        private:
            std::shared_ptr<DeviceManager>     device_;
            VkRenderPass                       render_pass_ = VK_NULL_HANDLE;
            std::vector<FramebufferAttachment> attachments_;
            uint32_t                           width_  = 0;
            uint32_t                           height_ = 0;
            uint32_t                           layers_ = 1;
    };

    // Framebuffer pool for managing multiple framebuffers (e.g., for swap chain images)
    class FramebufferPool
    {
        public:
            explicit FramebufferPool(std::shared_ptr<DeviceManager> device);
            ~FramebufferPool() = default;

            // Create framebuffers for all swap chain images
            void create_for_swap_chain(
                VkRenderPass                    render_pass,
                const std::vector<VkImageView>& image_views,
                uint32_t                        width,
                uint32_t                        height,
                VkImageView                     depth_view = VK_NULL_HANDLE);

            // Get framebuffer for specific image index
            Framebuffer* get_framebuffer(uint32_t index);

            // Clear all framebuffers
            void clear();

            uint32_t count() const { return static_cast<uint32_t>(framebuffers_.size()); }

        private:
            std::shared_ptr<DeviceManager>            device_;
            std::vector<std::unique_ptr<Framebuffer>> framebuffers_;
    };
} // namespace vulkan_engine::vulkan
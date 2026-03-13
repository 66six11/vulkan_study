#include "vulkan/resources/Framebuffer.hpp"
#include "vulkan/utils/VulkanError.hpp"

namespace vulkan_engine::vulkan
{
    // Framebuffer implementation
    Framebuffer::Framebuffer(
        std::shared_ptr<DeviceManager> device,
        const FramebufferConfig&       config)
        : device_(std::move(device))
        , render_pass_(config.render_pass)
        , width_(config.width)
        , height_(config.height)
        , layers_(config.layers)
    {
        std::vector<VkImageView> attachments;
        attachments.reserve(config.attachments.size());
        for (const auto& attach : config.attachments)
        {
            attachments.push_back(attach.image_view);
        }

        create_framebuffer(config.render_pass, attachments);
    }

    Framebuffer::Framebuffer(
        std::shared_ptr<DeviceManager>  device,
        VkRenderPass                    render_pass,
        const std::vector<VkImageView>& attachments,
        uint32_t                        width,
        uint32_t                        height,
        uint32_t                        layers)
        : device_(std::move(device))
        , render_pass_(render_pass)
        , width_(width)
        , height_(height)
        , layers_(layers)
    {
        create_framebuffer(render_pass, attachments);
    }

    Framebuffer::~Framebuffer()
    {
        if (framebuffer_ != VK_NULL_HANDLE && device_)
        {
            vkDestroyFramebuffer(device_->device(), framebuffer_, nullptr);
        }
    }

    Framebuffer::Framebuffer(Framebuffer&& other) noexcept
        : device_(std::move(other.device_))
        , framebuffer_(other.framebuffer_)
        , render_pass_(other.render_pass_)
        , width_(other.width_)
        , height_(other.height_)
        , layers_(other.layers_)
    {
        other.framebuffer_ = VK_NULL_HANDLE;
    }

    Framebuffer& Framebuffer::operator=(Framebuffer&& other) noexcept
    {
        if (this != &other)
        {
            if (framebuffer_ != VK_NULL_HANDLE && device_)
            {
                vkDestroyFramebuffer(device_->device(), framebuffer_, nullptr);
            }

            device_      = std::move(other.device_);
            framebuffer_ = other.framebuffer_;
            render_pass_ = other.render_pass_;
            width_       = other.width_;
            height_      = other.height_;
            layers_      = other.layers_;

            other.framebuffer_ = VK_NULL_HANDLE;
        }
        return *this;
    }

    void Framebuffer::create_framebuffer(
        VkRenderPass                    render_pass,
        const std::vector<VkImageView>& attachments)
    {
        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass      = render_pass;
        framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebuffer_info.pAttachments    = attachments.data();
        framebuffer_info.width           = width_;
        framebuffer_info.height          = height_;
        framebuffer_info.layers          = layers_;

        VkResult result = vkCreateFramebuffer(
                                              device_->device(),
                                              &framebuffer_info,
                                              nullptr,
                                              &framebuffer_);

        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create framebuffer", __FILE__, __LINE__);
        }
    }

    // FramebufferBuilder implementation
    FramebufferBuilder::FramebufferBuilder(std::shared_ptr<DeviceManager> device)
        : device_(std::move(device))
    {
    }

    FramebufferBuilder& FramebufferBuilder::render_pass(VkRenderPass render_pass)
    {
        render_pass_ = render_pass;
        return *this;
    }

    FramebufferBuilder& FramebufferBuilder::attachment(VkImageView image_view)
    {
        FramebufferAttachment attach{};
        attach.image_view = image_view;
        attachments_.push_back(attach);
        return *this;
    }

    FramebufferBuilder& FramebufferBuilder::attachment(VkImageView image_view, VkFormat format)
    {
        FramebufferAttachment attach{};
        attach.image_view = image_view;
        attach.format     = format;
        attachments_.push_back(attach);
        return *this;
    }

    FramebufferBuilder& FramebufferBuilder::dimensions(uint32_t width, uint32_t height)
    {
        width_  = width;
        height_ = height;
        return *this;
    }

    FramebufferBuilder& FramebufferBuilder::layers(uint32_t layers)
    {
        layers_ = layers;
        return *this;
    }

    std::unique_ptr<Framebuffer> FramebufferBuilder::build()
    {
        FramebufferConfig config{};
        config.render_pass = render_pass_;
        config.width       = width_;
        config.height      = height_;
        config.layers      = layers_;
        config.attachments = attachments_;

        return std::make_unique < Framebuffer > (device_, config);
    }

    // FramebufferPool implementation
    FramebufferPool::FramebufferPool(std::shared_ptr<DeviceManager> device)
        : device_(std::move(device))
    {
    }

    void FramebufferPool::create_for_swap_chain(
        VkRenderPass                    render_pass,
        const std::vector<VkImageView>& image_views,
        uint32_t                        width,
        uint32_t                        height)
    {
        clear();

        framebuffers_.reserve(image_views.size());
        for (auto image_view : image_views)
        {
            framebuffers_.push_back(
                                    std::make_unique < Framebuffer > (
                                        device_,
                                        render_pass,
                                        std::vector<VkImageView>{image_view},
                                        width,
                                        height,
                                        1));
        }
    }

    Framebuffer* FramebufferPool::get_framebuffer(uint32_t index)
    {
        if (index < framebuffers_.size())
        {
            return framebuffers_[index].get();
        }
        return nullptr;
    }

    void FramebufferPool::clear()
    {
        framebuffers_.clear();
    }
} // namespace vulkan_engine::vulkan
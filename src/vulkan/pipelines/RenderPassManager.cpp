#include "vulkan/pipelines/RenderPassManager.hpp"
#include "vulkan/utils/VulkanError.hpp"
#include "core/utils/Logger.hpp"

namespace vulkan_engine::vulkan
{
    RenderPassManager::RenderPassManager(std::shared_ptr<DeviceManager> device)
        : device_(std::move(device))
    {
    }

    RenderPassManager::~RenderPassManager()
    {
        clear();
    }

    RenderPassManager::RenderPassManager(RenderPassManager&& other) noexcept
        : device_(std::move(other.device_))
        , cache_(std::move(other.cache_))
    {
    }

    RenderPassManager& RenderPassManager::operator=(RenderPassManager&& other) noexcept
    {
        if (this != &other)
        {
            clear();
            device_ = std::move(other.device_);
            cache_  = std::move(other.cache_);
        }
        return *this;
    }

    VkRenderPass RenderPassManager::get_present_render_pass(VkFormat color_format)
    {
        RenderPassKey key{
            .color_format = color_format,
            .depth_format = VK_FORMAT_UNDEFINED,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .subpass_count = 1,
            .offscreen = false
        };
        return get_or_create(key);
    }

    VkRenderPass RenderPassManager::get_present_render_pass_with_depth(VkFormat color_format, VkFormat depth_format)
    {
        RenderPassKey key{
            .color_format = color_format,
            .depth_format = depth_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .subpass_count = 1,
            .offscreen = false
        };
        return get_or_create(key);
    }

    VkRenderPass RenderPassManager::get_offscreen_render_pass(VkFormat color_format, VkFormat depth_format)
    {
        RenderPassKey key{
            .color_format = color_format,
            .depth_format = depth_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .subpass_count = 1,
            .offscreen = true
        };
        return get_or_create(key);
    }

    VkRenderPass RenderPassManager::get_shadow_render_pass(VkFormat depth_format)
    {
        RenderPassKey key{
            .color_format = VK_FORMAT_UNDEFINED,
            .depth_format = depth_format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .subpass_count = 1,
            .offscreen = true
        };
        return get_or_create(key);
    }

    VkRenderPass RenderPassManager::get_or_create(const RenderPassKey& key)
    {
        auto it = cache_.find(key);
        if (it != cache_.end())
        {
            return it->second;
        }

        VkRenderPass render_pass = VK_NULL_HANDLE;

        if (key.offscreen)
        {
            // Offscreen render pass
            if (key.color_format != VK_FORMAT_UNDEFINED && key.depth_format != VK_FORMAT_UNDEFINED)
            {
                render_pass = create_offscreen_render_pass(key.color_format, key.depth_format);
            }
            else if (key.depth_format != VK_FORMAT_UNDEFINED)
            {
                render_pass = create_shadow_render_pass(key.depth_format);
            }
        }
        else
        {
            // Present render pass
            if (key.depth_format != VK_FORMAT_UNDEFINED)
            {
                render_pass = create_present_render_pass_with_depth(key.color_format, key.depth_format);
            }
            else
            {
                render_pass = create_present_render_pass(key.color_format);
            }
        }

        if (render_pass != VK_NULL_HANDLE)
        {
            cache_[key] = render_pass;
            logger::info("Created RenderPass [color: " + std::to_string(key.color_format) +
                         ", depth: " + std::to_string(key.depth_format) +
                         ", offscreen: " + std::to_string(key.offscreen) + "]");
        }

        return render_pass;
    }

    void RenderPassManager::cleanup_unused()
    {
        // 目前简单实现：清空所有缓存
        // 后续可以实现引用计数
        clear();
    }

    void RenderPassManager::clear()
    {
        if (!device_)
            return;

        VkDevice device = device_->device();
        vkDeviceWaitIdle(device);

        for (auto& [key, render_pass] : cache_)
        {
            if (render_pass != VK_NULL_HANDLE)
            {
                vkDestroyRenderPass(device, render_pass, nullptr);
            }
        }
        cache_.clear();

        logger::info("RenderPassManager cleared");
    }

    VkRenderPass RenderPassManager::create_present_render_pass(VkFormat color_format)
    {
        VkAttachmentDescription color_attachment{};
        color_attachment.format         = color_format;
        color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference color_ref{};
        color_ref.attachment = 0;
        color_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments    = &color_ref;

        VkSubpassDependency dependency{};
        dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass    = 0;
        dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        return create_render_pass({color_attachment}, {subpass}, {dependency});
    }

    VkRenderPass RenderPassManager::create_present_render_pass_with_depth(VkFormat color_format, VkFormat depth_format)
    {
        std::vector<VkAttachmentDescription> attachments(2);

        // Color attachment
        attachments[0].format         = color_format;
        attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Depth attachment
        attachments[1].format         = depth_format;
        attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference color_ref{};
        color_ref.attachment = 0;
        color_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depth_ref{};
        depth_ref.attachment = 1;
        depth_ref.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = 1;
        subpass.pColorAttachments       = &color_ref;
        subpass.pDepthStencilAttachment = &depth_ref;

        VkSubpassDependency dependencies[2] = {};

        // First dependency: wait for color attachment output
        dependencies[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass    = 0;
        dependencies[0].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask = 0;
        dependencies[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // Second dependency: wait for early fragment tests (depth)
        dependencies[1].srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependencies[1].dstSubpass    = 0;
        dependencies[1].srcStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[1].srcAccessMask = 0;
        dependencies[1].dstStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        return create_render_pass(attachments, {subpass}, {dependencies[0], dependencies[1]});
    }

    VkRenderPass RenderPassManager::create_offscreen_render_pass(VkFormat color_format, VkFormat depth_format)
    {
        std::vector<VkAttachmentDescription> attachments(2);

        // Color attachment - 最终布局为 SHADER_READ_ONLY
        attachments[0].format         = color_format;
        attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        // Depth attachment
        attachments[1].format         = depth_format;
        attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference color_ref{};
        color_ref.attachment = 0;
        color_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depth_ref{};
        depth_ref.attachment = 1;
        depth_ref.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = 1;
        subpass.pColorAttachments       = &color_ref;
        subpass.pDepthStencilAttachment = &depth_ref;

        VkSubpassDependency dependencies[2] = {};

        // First dependency: wait for fragment shader (texture read)
        dependencies[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass    = 0;
        dependencies[0].srcStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependencies[0].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        // Second dependency: wait for early fragment tests (depth)
        dependencies[1].srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependencies[1].dstSubpass    = 0;
        dependencies[1].srcStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[1].srcAccessMask = 0;
        dependencies[1].dstStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependencies[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        return create_render_pass(attachments, {subpass}, {dependencies[0], dependencies[1]});
    }

    VkRenderPass RenderPassManager::create_shadow_render_pass(VkFormat depth_format)
    {
        VkAttachmentDescription depth_attachment{};
        depth_attachment.format         = depth_format;
        depth_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        depth_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkAttachmentReference depth_ref{};
        depth_ref.attachment = 0;
        depth_ref.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.pDepthStencilAttachment = &depth_ref;

        VkSubpassDependency dependency{};
        dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass    = 0;
        dependency.srcStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependency.dstStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        return create_render_pass({depth_attachment}, {subpass}, {dependency});
    }

    VkRenderPass RenderPassManager::create_render_pass(
        const std::vector<VkAttachmentDescription>& attachments,
        const std::vector<VkSubpassDescription>&    subpasses,
        const std::vector<VkSubpassDependency>&     dependencies)
    {
        VkRenderPassCreateInfo render_pass_info{};
        render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
        render_pass_info.pAttachments    = attachments.data();
        render_pass_info.subpassCount    = static_cast<uint32_t>(subpasses.size());
        render_pass_info.pSubpasses      = subpasses.data();
        render_pass_info.dependencyCount = static_cast<uint32_t>(dependencies.size());
        render_pass_info.pDependencies   = dependencies.data();

        VkRenderPass render_pass = VK_NULL_HANDLE;
        VK_CHECK(vkCreateRenderPass(device_->device(), &render_pass_info, nullptr, &render_pass));

        return render_pass;
    }
} // namespace vulkan_engine::vulkan
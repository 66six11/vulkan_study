#pragma once

// Compatibility layer: RenderPass management
// Note: With Dynamic Rendering (Vulkan 1.3+), VkRenderPass is optional
// This provides a minimal compatibility layer

#include "engine/rhi/Device.hpp"
#include <vulkan/vulkan.h>
#include <unordered_map>
#include <string>

namespace engine::vulkan
{
    // RenderPassManager - caches and manages render passes
    // For Dynamic Rendering, most of this becomes unnecessary
    class RenderPassManager
    {
        public:
            RenderPassManager() = default;

            explicit RenderPassManager(std::shared_ptr<rhi::Device> device)
                : device_(std::move(device))
            {
            }

            ~RenderPassManager()
            {
                clear();
            }

            // Get or create a present render pass (for swap chain)
            VkRenderPass get_present_render_pass(VkFormat color_format)
            {
                auto key = make_key("present", color_format, VK_FORMAT_UNDEFINED);
                auto it  = renderPasses_.find(key);
                if (it != renderPasses_.end())
                {
                    return it->second;
                }

                VkRenderPass pass = create_present_render_pass(color_format);
                if (pass != VK_NULL_HANDLE)
                {
                    renderPasses_[key] = pass;
                }
                return pass;
            }

            // Get or create a present render pass with depth
            VkRenderPass get_present_render_pass_with_depth(VkFormat color_format, VkFormat depth_format)
            {
                auto key = make_key("present_depth", color_format, depth_format);
                auto it  = renderPasses_.find(key);
                if (it != renderPasses_.end())
                {
                    return it->second;
                }

                VkRenderPass pass = create_present_render_pass_with_depth(color_format, depth_format);
                if (pass != VK_NULL_HANDLE)
                {
                    renderPasses_[key] = pass;
                }
                return pass;
            }

            // Get or create an offscreen render pass
            VkRenderPass get_offscreen_render_pass(VkFormat color_format, VkFormat depth_format)
            {
                auto key = make_key("offscreen", color_format, depth_format);
                auto it  = renderPasses_.find(key);
                if (it != renderPasses_.end())
                {
                    return it->second;
                }

                VkRenderPass pass = create_offscreen_render_pass(color_format, depth_format);
                if (pass != VK_NULL_HANDLE)
                {
                    renderPasses_[key] = pass;
                }
                return pass;
            }

            void clear()
            {
                if (!device_) return;
                VkDevice vkDevice = static_cast<VkDevice>(device_->nativeDevice());
                for (auto& [key, pass] : renderPasses_)
                {
                    if (pass != VK_NULL_HANDLE)
                    {
                        vkDestroyRenderPass(vkDevice, pass, nullptr);
                    }
                }
                renderPasses_.clear();
            }

        private:
            std::string make_key(const char* type, VkFormat color, VkFormat depth)
            {
                return std::string(type) + "_" + std::to_string(color) + "_" + std::to_string(depth);
            }

            VkRenderPass create_present_render_pass(VkFormat color_format)
            {
                VkDevice vkDevice = static_cast<VkDevice>(device_->nativeDevice());

                VkAttachmentDescription colorAttachment{};
                colorAttachment.format         = color_format;
                colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
                colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
                colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
                colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
                colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

                VkAttachmentReference colorRef{};
                colorRef.attachment = 0;
                colorRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

                VkSubpassDescription subpass{};
                subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass.colorAttachmentCount = 1;
                subpass.pColorAttachments    = &colorRef;

                VkSubpassDependency dependency{};
                dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
                dependency.dstSubpass    = 0;
                dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.srcAccessMask = 0;
                dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

                VkRenderPassCreateInfo createInfo{};
                createInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                createInfo.attachmentCount = 1;
                createInfo.pAttachments    = &colorAttachment;
                createInfo.subpassCount    = 1;
                createInfo.pSubpasses      = &subpass;
                createInfo.dependencyCount = 1;
                createInfo.pDependencies   = &dependency;

                VkRenderPass pass = VK_NULL_HANDLE;
                vkCreateRenderPass(vkDevice, &createInfo, nullptr, &pass);
                return pass;
            }

            VkRenderPass create_present_render_pass_with_depth(VkFormat color_format, VkFormat depth_format)
            {
                VkDevice vkDevice = static_cast<VkDevice>(device_->nativeDevice());

                std::array<VkAttachmentDescription, 2> attachments{};

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

                VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
                VkAttachmentReference depthRef{1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

                VkSubpassDescription subpass{};
                subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass.colorAttachmentCount    = 1;
                subpass.pColorAttachments       = &colorRef;
                subpass.pDepthStencilAttachment = &depthRef;

                std::array<VkSubpassDependency, 2> dependencies{};
                dependencies[0].srcSubpass    = VK_SUBPASS_EXTERNAL;
                dependencies[0].dstSubpass    = 0;
                dependencies[0].srcStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                dependencies[0].dstStageMask  = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                dependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

                dependencies[1].srcSubpass    = VK_SUBPASS_EXTERNAL;
                dependencies[1].dstSubpass    = 0;
                dependencies[1].srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[1].dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                dependencies[1].srcAccessMask = 0;
                dependencies[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

                VkRenderPassCreateInfo createInfo{};
                createInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
                createInfo.pAttachments    = attachments.data();
                createInfo.subpassCount    = 1;
                createInfo.pSubpasses      = &subpass;
                createInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
                createInfo.pDependencies   = dependencies.data();

                VkRenderPass pass = VK_NULL_HANDLE;
                vkCreateRenderPass(vkDevice, &createInfo, nullptr, &pass);
                return pass;
            }

            VkRenderPass create_offscreen_render_pass(VkFormat color_format, VkFormat depth_format)
            {
                VkDevice vkDevice = static_cast<VkDevice>(device_->nativeDevice());

                std::vector<VkAttachmentDescription> attachments;

                // Color attachment
                VkAttachmentDescription colorAttachment{};
                colorAttachment.format         = color_format;
                colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
                colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
                colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
                colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
                colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                attachments.push_back(colorAttachment);

                VkAttachmentReference colorRef{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
                VkAttachmentReference depthRef{};

                if (depth_format != VK_FORMAT_UNDEFINED)
                {
                    VkAttachmentDescription depthAttachment{};
                    depthAttachment.format         = depth_format;
                    depthAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
                    depthAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
                    depthAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                    depthAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                    depthAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
                    depthAttachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                    attachments.push_back(depthAttachment);

                    depthRef = {1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
                }

                VkSubpassDescription subpass{};
                subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
                subpass.colorAttachmentCount = 1;
                subpass.pColorAttachments    = &colorRef;
                if (depth_format != VK_FORMAT_UNDEFINED)
                {
                    subpass.pDepthStencilAttachment = &depthRef;
                }

                VkSubpassDependency dependency{};
                dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
                dependency.dstSubpass    = 0;
                dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                dependency.srcAccessMask = 0;
                dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                VkRenderPassCreateInfo createInfo{};
                createInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
                createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
                createInfo.pAttachments    = attachments.data();
                createInfo.subpassCount    = 1;
                createInfo.pSubpasses      = &subpass;
                createInfo.dependencyCount = 1;
                createInfo.pDependencies   = &dependency;

                VkRenderPass pass = VK_NULL_HANDLE;
                vkCreateRenderPass(vkDevice, &createInfo, nullptr, &pass);
                return pass;
            }

            std::shared_ptr<rhi::Device>                  device_;
            std::unordered_map<std::string, VkRenderPass> renderPasses_;
    };
} // namespace engine::vulkan
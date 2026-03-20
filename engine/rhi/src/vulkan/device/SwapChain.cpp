#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#undef min
#undef max
#endif

#include "vulkan/device/SwapChain.hpp"
#include "platform/windowing/Window.hpp"
#include "vulkan/utils/VulkanError.hpp"
#include <algorithm>
#include <limits>

namespace vulkan_engine::vulkan
{
    uint32_t SwapChain::default_image_index_ = 0;

    SwapChain::SwapChain(
        std::shared_ptr<DeviceManager>                   device,
        std::shared_ptr<vulkan_engine::platform::Window> window,
        const SwapChainConfig&                           config)
        : device_(std::move(device))
        , window_(std::move(window))
        , config_(config)
    {
    }

    SwapChain::~SwapChain()
    {
        shutdown();
    }

    SwapChain::SwapChain(SwapChain&& other) noexcept
        : device_(std::move(other.device_))
        , window_(std::move(other.window_))
        , config_(other.config_)
        , surface_(other.surface_)
        , swap_chain_(other.swap_chain_)
        , default_render_pass_(other.default_render_pass_)
        , format_(other.format_)
        , color_space_(other.color_space_)
        , present_mode_(other.present_mode_)
        , extent_(other.extent_)
        , transform_(other.transform_)
        , images_(std::move(other.images_))
        , graphics_queue_family_(other.graphics_queue_family_)
        , present_queue_family_(other.present_queue_family_)
        , queues_are_same_(other.queues_are_same_)
        , needs_recreation_(other.needs_recreation_)
        , recreate_callback_(std::move(other.recreate_callback_))
    {
        other.surface_             = VK_NULL_HANDLE;
        other.swap_chain_          = VK_NULL_HANDLE;
        other.default_render_pass_ = VK_NULL_HANDLE;
    }

    SwapChain& SwapChain::operator=(SwapChain&& other) noexcept
    {
        if (this != &other)
        {
            shutdown();

            device_                = std::move(other.device_);
            window_                = std::move(other.window_);
            config_                = other.config_;
            surface_               = other.surface_;
            swap_chain_            = other.swap_chain_;
            default_render_pass_   = other.default_render_pass_;
            format_                = other.format_;
            color_space_           = other.color_space_;
            present_mode_          = other.present_mode_;
            extent_                = other.extent_;
            transform_             = other.transform_;
            images_                = std::move(other.images_);
            graphics_queue_family_ = other.graphics_queue_family_;
            present_queue_family_  = other.present_queue_family_;
            queues_are_same_       = other.queues_are_same_;
            needs_recreation_      = other.needs_recreation_;
            recreate_callback_     = std::move(other.recreate_callback_);

            other.surface_             = VK_NULL_HANDLE;
            other.swap_chain_          = VK_NULL_HANDLE;
            other.default_render_pass_ = VK_NULL_HANDLE;
        }
        return *this;
    }

    bool SwapChain::initialize()
    {
        if (!create_surface())
        {
            return false;
        }

        if (!select_surface_format())
        {
            return false;
        }

        if (!select_present_mode())
        {
            return false;
        }

        if (!select_extent())
        {
            return false;
        }

        // Find queue families
        graphics_queue_family_ = 0; // Assume graphics queue is at index 0 from DeviceManager
        present_queue_family_  = find_present_queue_family();
        queues_are_same_       = (graphics_queue_family_ == present_queue_family_);

        if (!create_swap_chain())
        {
            return false;
        }

        if (!create_image_views())
        {
            return false;
        }

        if (!create_default_render_pass())
        {
            return false;
        }

        return true;
    }

    void SwapChain::shutdown()
    {
        if (device_&& device_
        
        ->
        device()
        )
        {
            vkDeviceWaitIdle(device_->device());
        }

        cleanup_image_views();

        if (default_render_pass_ != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(device_->device(), default_render_pass_, nullptr);
            default_render_pass_ = VK_NULL_HANDLE;
        }

        cleanup_swap_chain();

        if (surface_ != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(device_->instance(), surface_, nullptr);
            surface_ = VK_NULL_HANDLE;
        }
    }

    bool SwapChain::recreate()
    {
        // Wait for device to be idle
        vkDeviceWaitIdle(device_->device());

        // Cleanup old render pass
        if (default_render_pass_ != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(device_->device(), default_render_pass_, nullptr);
            default_render_pass_ = VK_NULL_HANDLE;
        }

        // Cleanup old swap chain
        cleanup_image_views();
        cleanup_swap_chain();

        // Get new window size
        auto [width, height] = window_->size();
        if (width == 0 || height == 0)
        {
            // Window is minimized, delay recreation
            return false;
        }

        // Recreate
        if (!select_extent())
        {
            return false;
        }

        if (!create_swap_chain())
        {
            return false;
        }

        if (!create_image_views())
        {
            return false;
        }

        if (!create_default_render_pass())
        {
            return false;
        }

        needs_recreation_ = false;

        // Notify callback
        if (recreate_callback_)
        {
            recreate_callback_(extent_.width, extent_.height);
        }

        return true;
    }

    bool SwapChain::acquire_next_image(
        VkSemaphore image_available_semaphore,
        VkFence     fence,
        uint32_t&   out_image_index)
    {
        VkResult result = vkAcquireNextImageKHR(
                                                device_->device(),
                                                swap_chain_,
                                                UINT64_MAX,
                                                // No timeout
                                                image_available_semaphore,
                                                fence,
                                                &out_image_index);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            needs_recreation_ = true;
            return false;
        }
        else if (result == VK_SUBOPTIMAL_KHR)
        {
            needs_recreation_ = true;
            // Can still present, but should recreate after
        }
        else if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to acquire next swap chain image", __FILE__, __LINE__);
        }

        return true;
    }

    bool SwapChain::present(
        VkQueue     present_queue,
        uint32_t    image_index,
        VkSemaphore render_finished_semaphore,
        VkResult*   out_present_result)
    {
        VkPresentInfoKHR present_info{};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        VkSemaphore wait_semaphores[]   = {render_finished_semaphore};
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores    = wait_semaphores;

        VkSwapchainKHR swap_chains[] = {swap_chain_};
        present_info.swapchainCount  = 1;
        present_info.pSwapchains     = swap_chains;
        present_info.pImageIndices   = &image_index;

        VkResult result = vkQueuePresentKHR(present_queue, &present_info);

        if (out_present_result)
        {
            *out_present_result = result;
        }

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        {
            needs_recreation_ = true;
            return result == VK_SUBOPTIMAL_KHR; // Suboptimal is still success
        }
        else if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to present swap chain image", __FILE__, __LINE__);
        }

        return true;
    }

    void SwapChain::wait_for_present_queue()
    {
        vkQueueWaitIdle(device_->graphics_queue());
    }

    bool SwapChain::create_surface()
    {
        surface_ = window_->create_surface(device_->instance());
        return surface_ != VK_NULL_HANDLE;
    }

    bool SwapChain::select_surface_format()
    {
        auto formats = get_surface_formats();

        if (formats.empty())
        {
            return false;
        }

        // Check if preferred format is available
        for (const auto& format : formats)
        {
            if (format.format == config_.preferred_format &&
                format.colorSpace == config_.preferred_color_space)
            {
                format_      = format.format;
                color_space_ = format.colorSpace;
                return true;
            }
        }

        // Fall back to first available
        format_      = formats[0].format;
        color_space_ = formats[0].colorSpace;
        return true;
    }

    bool SwapChain::select_present_mode()
    {
        auto modes = get_present_modes();

        if (modes.empty())
        {
            return false;
        }

        // Check if preferred mode is available
        for (const auto& mode : modes)
        {
            if (mode == config_.preferred_present_mode)
            {
                present_mode_ = mode;
                return true;
            }
        }

        // FIFO is guaranteed to be available
        present_mode_ = VK_PRESENT_MODE_FIFO_KHR;
        return true;
    }

    bool SwapChain::select_extent()
    {
        VkSurfaceCapabilitiesKHR capabilities = get_surface_capabilities();

        if (capabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)())
        {
            extent_ = capabilities.currentExtent;
        }
        else
        {
            auto     window_size = window_->size();
            uint32_t width       = window_size.first;
            uint32_t height      = window_size.second;

            VkExtent2D actual_extent{};
            actual_extent.width  = static_cast<uint32_t>(width);
            actual_extent.height = static_cast<uint32_t>(height);

            actual_extent.width = (std::max)(
                                             capabilities.minImageExtent.width,
                                             (std::min)(actual_extent.width, capabilities.maxImageExtent.width));

            actual_extent.height = (std::max)(
                                              capabilities.minImageExtent.height,
                                              (std::min)(actual_extent.height, capabilities.maxImageExtent.height));

            extent_ = actual_extent;
        }

        transform_ = capabilities.currentTransform;

        return extent_.width > 0 && extent_.height > 0;
    }

    bool SwapChain::create_swap_chain()
    {
        VkSurfaceCapabilitiesKHR capabilities = get_surface_capabilities();

        uint32_t image_count = config_.preferred_image_count;
        image_count          = (std::max)(capabilities.minImageCount,
                                 (std::min)(image_count, capabilities.maxImageCount));

        VkSwapchainCreateInfoKHR create_info{};
        create_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.surface          = surface_;
        create_info.minImageCount    = image_count;
        create_info.imageFormat      = format_;
        create_info.imageColorSpace  = color_space_;
        create_info.imageExtent      = extent_;
        create_info.imageArrayLayers = 1;
        create_info.imageUsage       = config_.image_usage;

        uint32_t queue_family_indices[] = {graphics_queue_family_, present_queue_family_};

        if (queues_are_same_)
        {
            create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }
        else
        {
            create_info.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            create_info.queueFamilyIndexCount = 2;
            create_info.pQueueFamilyIndices   = queue_family_indices;
        }

        create_info.preTransform   = transform_;
        create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode    = present_mode_;
        create_info.clipped        = VK_TRUE;
        create_info.oldSwapchain   = VK_NULL_HANDLE;

        VkResult result = vkCreateSwapchainKHR(device_->device(), &create_info, nullptr, &swap_chain_);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create swap chain", __FILE__, __LINE__);
        }

        // Get swap chain images
        uint32_t actual_image_count = 0;
        vkGetSwapchainImagesKHR(device_->device(), swap_chain_, &actual_image_count, nullptr);

        std::vector<VkImage> vk_images(actual_image_count);
        vkGetSwapchainImagesKHR(device_->device(), swap_chain_, &actual_image_count, vk_images.data());

        images_.resize(actual_image_count);
        for (uint32_t i = 0; i < actual_image_count; i++)
        {
            images_[i].image = vk_images[i];
            images_[i].index = i;
        }

        return true;
    }

    bool SwapChain::create_image_views()
    {
        for (auto& image : images_)
        {
            VkImageViewCreateInfo create_info{};
            create_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            create_info.image    = image.image;
            create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            create_info.format   = format_;

            create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

            create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            create_info.subresourceRange.baseMipLevel   = 0;
            create_info.subresourceRange.levelCount     = 1;
            create_info.subresourceRange.baseArrayLayer = 0;
            create_info.subresourceRange.layerCount     = 1;

            VkResult result = vkCreateImageView(device_->device(), &create_info, nullptr, &image.view);
            if (result != VK_SUCCESS)
            {
                throw VulkanError(result, "Failed to create swap chain image view", __FILE__, __LINE__);
            }
        }

        return true;
    }

    void SwapChain::cleanup_image_views()
    {
        for (auto& image : images_)
        {
            if (image.view != VK_NULL_HANDLE)
            {
                vkDestroyImageView(device_->device(), image.view, nullptr);
                image.view = VK_NULL_HANDLE;
            }
        }
        images_.clear();
    }

    void SwapChain::cleanup_swap_chain()
    {
        if (swap_chain_ != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(device_->device(), swap_chain_, nullptr);
            swap_chain_ = VK_NULL_HANDLE;
        }
    }

    bool SwapChain::create_default_render_pass()
    {
        // Destroy existing render pass if any
        if (default_render_pass_ != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(device_->device(), default_render_pass_, nullptr);
            default_render_pass_ = VK_NULL_HANDLE;
        }

        VkAttachmentDescription color_attachment{};
        color_attachment.format         = format_;
        color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference color_attachment_ref{};
        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments    = &color_attachment_ref;

        VkSubpassDependency dependency{};
        dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass    = 0;
        dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo render_pass_info{};
        render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount = 1;
        render_pass_info.pAttachments    = &color_attachment;
        render_pass_info.subpassCount    = 1;
        render_pass_info.pSubpasses      = &subpass;
        render_pass_info.dependencyCount = 1;
        render_pass_info.pDependencies   = &dependency;

        VkResult result = vkCreateRenderPass(device_->device(), &render_pass_info, nullptr, &default_render_pass_);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create default render pass", __FILE__, __LINE__);
        }

        return true;
    }

    bool SwapChain::create_render_pass_with_depth(VkFormat depth_format)
    {
        // Destroy existing render pass if any
        if (default_render_pass_ != VK_NULL_HANDLE)
        {
            vkDestroyRenderPass(device_->device(), default_render_pass_, nullptr);
            default_render_pass_ = VK_NULL_HANDLE;
        }

        // Color attachment
        VkAttachmentDescription color_attachment{};
        color_attachment.format         = format_;
        color_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        color_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        color_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        color_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        color_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        color_attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        // Depth attachment
        VkAttachmentDescription depth_attachment{};
        depth_attachment.format         = depth_format;
        depth_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        depth_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depth_attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        depth_attachment.finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Attachment references
        VkAttachmentReference color_attachment_ref{};
        color_attachment_ref.attachment = 0;
        color_attachment_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depth_attachment_ref{};
        depth_attachment_ref.attachment = 1;
        depth_attachment_ref.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        // Subpass
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount    = 1;
        subpass.pColorAttachments       = &color_attachment_ref;
        subpass.pDepthStencilAttachment = &depth_attachment_ref;

        // Dependencies
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

        VkAttachmentDescription attachments[2] = {color_attachment, depth_attachment};

        VkRenderPassCreateInfo render_pass_info{};
        render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_info.attachmentCount = 2;
        render_pass_info.pAttachments    = attachments;
        render_pass_info.subpassCount    = 1;
        render_pass_info.pSubpasses      = &subpass;
        render_pass_info.dependencyCount = 2;
        render_pass_info.pDependencies   = dependencies;

        VkResult result = vkCreateRenderPass(device_->device(), &render_pass_info, nullptr, &default_render_pass_);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create render pass with depth", __FILE__, __LINE__);
        }

        return true;
    }

    std::vector<VkSurfaceFormatKHR> SwapChain::get_surface_formats() const
    {
        uint32_t count = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device_->physical_device(), surface_, &count, nullptr);

        std::vector<VkSurfaceFormatKHR> formats(count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device_->physical_device(), surface_, &count, formats.data());

        return formats;
    }

    std::vector<VkPresentModeKHR> SwapChain::get_present_modes() const
    {
        uint32_t count = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device_->physical_device(), surface_, &count, nullptr);

        std::vector<VkPresentModeKHR> modes(count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device_->physical_device(), surface_, &count, modes.data());

        return modes;
    }

    VkSurfaceCapabilitiesKHR SwapChain::get_surface_capabilities() const
    {
        VkSurfaceCapabilitiesKHR capabilities{};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                                                  device_->physical_device(),
                                                  surface_,
                                                  &capabilities);
        return capabilities;
    }

    uint32_t SwapChain::find_present_queue_family()
    {
        uint32_t queue_family_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device_->physical_device(), &queue_family_count, nullptr);

        for (uint32_t i = 0; i < queue_family_count; i++)
        {
            VkBool32 present_support = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device_->physical_device(), i, surface_, &present_support);

            if (present_support)
            {
                return i;
            }
        }

        // Fall back to graphics queue family
        return graphics_queue_family_;
    }
} // namespace vulkan_engine::vulkan
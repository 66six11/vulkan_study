#pragma once
#include "Platform.h"
#include "constants.h"
#include <vector>

struct SwapchainResources
{
    VkDevice     device      = VK_NULL_HANDLE;
    VkCommandPool commandPool = VK_NULL_HANDLE;
       
    VkSwapchainKHR               swapchain        = VK_NULL_HANDLE;
    std::vector<VkImage>         images;
    VkFormat                     imageFormat      = VK_FORMAT_UNDEFINED;
    VkExtent2D                   extent           = {0, 0};
    std::vector<VkImageView>     imageViews;
    VkRenderPass                 renderPass       = VK_NULL_HANDLE;
    VkPipelineLayout             pipelineLayout   = VK_NULL_HANDLE;
    VkPipeline                   graphicsPipeline = VK_NULL_HANDLE;
    std::vector<VkFramebuffer>   framebuffers;
    std::vector<VkCommandBuffer> commandBuffers;
       
    SwapchainResources() = default;
    SwapchainResources(VkDevice device, VkCommandPool pool);
       
    ~SwapchainResources();
       
    SwapchainResources(const SwapchainResources&)            = delete;
    SwapchainResources& operator=(const SwapchainResources&) = delete;
       
    SwapchainResources(SwapchainResources&& other) noexcept;
    SwapchainResources& operator=(SwapchainResources&& other) noexcept;
       
    void destroy();
       
    private:
        void moveFrom(SwapchainResources&& other) noexcept;
};




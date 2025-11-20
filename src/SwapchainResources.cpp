//
// Created by C66 on 2025/11/21.
//
#include "SwapchainResources.h"

SwapchainResources::SwapchainResources(VkDevice device_, VkCommandPool pool) :
    device(device_), commandPool(pool)
{
}

SwapchainResources::~SwapchainResources()
{
    destroy();
}

void SwapchainResources::destroy()
{
    if (device == VK_NULL_HANDLE)
        return;

    for (auto fb : framebuffers)
        vkDestroyFramebuffer(device, fb, nullptr);
    framebuffers.clear();

    if (graphicsPipeline)
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
    if (pipelineLayout)
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    if (renderPass)
        vkDestroyRenderPass(device, renderPass, nullptr);

    for (auto view : imageViews)
        vkDestroyImageView(device, view, nullptr);
    imageViews.clear();

    if (swapchain)
        vkDestroySwapchainKHR(device, swapchain, nullptr);
    swapchain = VK_NULL_HANDLE;

    images.clear();
    commandBuffers.clear();
    imageFormat = VK_FORMAT_UNDEFINED;
    extent      = {0, 0};
}

SwapchainResources::SwapchainResources(SwapchainResources&& other) noexcept
{
    moveFrom(std::move(other));
}

SwapchainResources& SwapchainResources::operator=(SwapchainResources&& other) noexcept
{
    if (this != &other)
    {
        destroy();
        moveFrom(std::move(other));
    }
    return *this;
}

void SwapchainResources::moveFrom(SwapchainResources&& other) noexcept
{
    device           = other.device;
    commandPool      = other.commandPool;
    swapchain        = other.swapchain;
    images           = std::move(other.images);
    imageFormat      = other.imageFormat;
    extent           = other.extent;
    imageViews       = std::move(other.imageViews);
    renderPass       = other.renderPass;
    pipelineLayout   = other.pipelineLayout;
    graphicsPipeline = other.graphicsPipeline;
    framebuffers     = std::move(other.framebuffers);
    commandBuffers   = std::move(other.commandBuffers);

    other.device           = VK_NULL_HANDLE;
    other.commandPool      = VK_NULL_HANDLE;
    other.swapchain        = VK_NULL_HANDLE;
    other.renderPass       = VK_NULL_HANDLE;
    other.pipelineLayout   = VK_NULL_HANDLE;
    other.graphicsPipeline = VK_NULL_HANDLE;
    other.imageFormat      = VK_FORMAT_UNDEFINED;
    other.extent           = {0, 0};
}

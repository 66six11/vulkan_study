#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "constants.h"
#include <vector>

// 命令缓冲和同步相关函数声明
void createCommandPool(VkDevice device, QueueFamilyIndices indices, VkCommandPool& commandPool);
void createCommandBuffers(VkDevice device, VkCommandPool commandPool, 
                         const std::vector<VkFramebuffer>& swapChainFramebuffers,
                         VkRenderPass renderPass, VkExtent2D swapChainExtent,
                         VkPipeline graphicsPipeline, const std::vector<VkImageView>& swapChainImageViews,
                         std::vector<VkCommandBuffer>& commandBuffers);
void createSemaphores(VkDevice device, VkSemaphore& imageAvailableSemaphore, VkSemaphore& renderFinishedSemaphore);
void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, 
                        VkRenderPass renderPass, VkExtent2D swapChainExtent,
                        VkPipeline graphicsPipeline, VkFramebuffer framebuffer);
void drawFrame(VkDevice device, VkSwapchainKHR swapChain, VkQueue graphicsQueue, VkQueue presentQueue,
               const std::vector<VkCommandBuffer>& commandBuffers,
               VkSemaphore imageAvailableSemaphore, VkSemaphore renderFinishedSemaphore);
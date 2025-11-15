#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "constants.h"
#include <vector>
#include <string>

// 渲染相关函数声明
void createRenderPass(VkDevice device, VkFormat swapChainImageFormat, VkRenderPass& renderPass);
void createGraphicsPipeline(VkDevice device, VkExtent2D swapChainExtent, VkRenderPass renderPass,
                           VkPipelineLayout& pipelineLayout, VkPipeline& graphicsPipeline);
void createFramebuffers(VkDevice device, const std::vector<VkImageView>& swapChainImageViews,
                       VkRenderPass renderPass, VkExtent2D swapChainExtent,
                       std::vector<VkFramebuffer>& swapChainFramebuffers);
VkShaderModule createShaderModule(VkDevice device, const std::string& code);
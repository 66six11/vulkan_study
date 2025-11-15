#pragma once

#include "HelloTriangleApplication.h"

// 命令缓冲和同步相关函数
void HelloTriangleApplication::createCommandPool();
void HelloTriangleApplication::createCommandBuffers();
void HelloTriangleApplication::createSemaphores();
void HelloTriangleApplication::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
void HelloTriangleApplication::drawFrame();
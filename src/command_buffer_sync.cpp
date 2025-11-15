#include "../include/HelloTriangleApplication.h"
#include "../include/command_buffer_sync.h"
#include <stdexcept>
#include <vector>
#include <cstddef>

// 创建命令池
// 创建命令池用于分配命令缓冲
void HelloTriangleApplication::createCommandPool() {
    // 找到图形队列族
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    // 填充命令池创建信息
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;                    // 结构体类型标识
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;               // 标志（允许重置命令缓冲）
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();          // 队列族索引

    // 创建命令池
    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

// 创建命令缓冲
// 分配命令缓冲并记录渲染命令
void HelloTriangleApplication::createCommandBuffers() {
    // 为每个帧缓冲分配命令缓冲
    commandBuffers.resize(swapChainFramebuffers.size());

    // 填充命令缓冲分配信息
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;     // 结构体类型标识
    allocInfo.commandPool = commandPool;                                  // 命令池
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;                    // 命令缓冲级别（主缓冲）
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();      // 命令缓冲数量

    // 分配命令缓冲
    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }

    // 记录每个命令缓冲
    for (size_t i = 0; i < commandBuffers.size(); i++) {
        recordCommandBuffer(commandBuffers[i], static_cast<uint32_t>(i));
    }
}

// 创建信号量
// 创建信号量用于同步渲染操作
void HelloTriangleApplication::createSemaphores() {
    // 创建信号量用于同步
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;  // 结构体类型标识

    // 创建图像可用信号量和渲染完成信号量
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {
        throw std::runtime_error("failed to create semaphores!");
    }
}

// 记录命令缓冲
// 在命令缓冲中记录渲染命令
void HelloTriangleApplication::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    // 填充命令缓冲开始信息
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;  // 结构体类型标识
    beginInfo.flags = 0; // 可选的开始标志
    beginInfo.pInheritanceInfo = nullptr; // 仅用于二级命令缓冲

    // 开始记录命令缓冲
    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    // 填充渲染通道开始信息
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;            // 结构体类型标识
    renderPassInfo.renderPass = renderPass;                                     // 渲染通道
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];             // 帧缓冲
    renderPassInfo.renderArea.offset = {0, 0};                                  // 渲染区域偏移
    renderPassInfo.renderArea.extent = swapChainExtent;                         // 渲染区域尺寸

    // 设置清除颜色（黑色不透明）
    VkClearValue clearColor = {{0.0f, 0.0f, 0.0f, 1.0f}};
    renderPassInfo.clearValueCount = 1;                                         // 清除值数量
    renderPassInfo.pClearValues = &clearColor;                                  // 指向清除值

    // 开始渲染通道
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // 绑定图形管线（虽然目前是空的）
    if (graphicsPipeline != VK_NULL_HANDLE) {
        // 绑定图形管线
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    }

    // 绘制三角形（3个顶点，1个实例，从索引0开始，实例从索引0开始）
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    // 结束渲染通道
    vkCmdEndRenderPass(commandBuffer);

    // 结束记录命令缓冲
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

// 绘制一帧
// 执行绘制操作并呈现结果
void HelloTriangleApplication::drawFrame() {
    // 获取下一个可用的交换链图像索引
    uint32_t imageIndex;
    // 等待图像可用信号量，无限期等待（UINT64_MAX）
    vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    // 填充命令提交信息
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;  // 结构体类型标识

    // 设置等待信号量
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
    // 设置等待阶段（颜色附件输出阶段）
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;                          // 等待信号量数量
    submitInfo.pWaitSemaphores = waitSemaphores;                // 指向等待信号量
    submitInfo.pWaitDstStageMask = waitStages;                  // 指向等待阶段
    submitInfo.commandBufferCount = 1;                          // 命令缓冲数量
    submitInfo.pCommandBuffers = &commandBuffers[imageIndex];   // 指向命令缓冲

    // 设置信号量
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;                        // 信号量数量
    submitInfo.pSignalSemaphores = signalSemaphores;            // 指向信号量

    // 向图形队列提交命令
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    // 填充呈现信息
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;     // 结构体类型标识
    presentInfo.waitSemaphoreCount = 1;                         // 等待信号量数量
    presentInfo.pWaitSemaphores = signalSemaphores;             // 指向等待信号量

    // 设置交换链
    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;                             // 交换链数量
    presentInfo.pSwapchains = swapChains;                       // 指向交换链
    presentInfo.pImageIndices = &imageIndex;                    // 指向图像索引
    presentInfo.pResults = nullptr; // 可选                         // 指向结果（可选）

    // 向呈现队列提交呈现请求
    vkQueuePresentKHR(presentQueue, &presentInfo);
}
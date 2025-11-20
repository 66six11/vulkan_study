#include "command_buffer_sync.h"
#include <stdexcept>
#include <vector>

/**
 * @brief 创建命令池
 * 
 * 创建命令池对象，用于分配命令缓冲，管理命令缓冲的内存
 * 
 * @param device 逻辑设备
 * @param indices 队列族索引
 * @param commandPool [out] 创建的命令池对象
 */
void createCommandPool(VkDevice device, QueueFamilyIndices indices, VkCommandPool &commandPool) {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = indices.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

/**
 * @brief 创建命令缓冲
 * 
 * 从命令池中分配命令缓冲对象，用于记录命令序列
 * 
 * @param device 逻辑设备
 * @param commandPool 命令池
 * @param swapChainFramebuffers 交换链帧缓冲集合
 * @param renderPass 渲染通道
 * @param swapChainExtent 交换链图像尺寸
 * @param graphicsPipeline 图形管线
 * @param swapChainImageViews 交换链图像视图集合
 * @param commandBuffers [out] 创建的命令缓冲集合
 */
void createCommandBuffers(VkDevice                          device,
                          VkCommandPool                     commandPool,
                          const std::vector<VkFramebuffer> &swapChainFramebuffers,
                          VkRenderPass                      renderPass,
                          VkExtent2D                        swapChainExtent,
                          VkPipeline                        graphicsPipeline,
                          const std::vector<VkImageView> &  swapChainImageViews,
                          std::vector<VkCommandBuffer> &    commandBuffers) {
    commandBuffers.resize(swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = commandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

/**
 * @brief 创建信号量
 * 
 * 创建用于同步操作的信号量对象
 * 
 * @param device 逻辑设备
 * @param imageAvailableSemaphore [out] 图像可用信号量
 * @param renderFinishedSemaphore [out] 渲染完成信号量
 */
void createSemaphores(VkDevice device, VkSemaphore &imageAvailableSemaphore, VkSemaphore &renderFinishedSemaphore) {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS) {
        throw std::runtime_error("failed to create semaphores!");
    }
}

/**
 * @brief 记录命令缓冲
 * 
 * 在命令缓冲中记录渲染命令，包括开始渲染通道、绑定管线、绘制命令和结束渲染通道
 * 
 * @param commandBuffer 要记录的命令缓冲
 * @param imageIndex 图像索引
 * @param renderPass 渲染通道
 * @param swapChainExtent 交换链图像尺寸
 * @param graphicsPipeline 图形管线
 * @param framebuffer 帧缓冲
 */
void recordCommandBuffer(VkCommandBuffer commandBuffer,
                         uint32_t        imageIndex,
                         VkRenderPass    renderPass,
                         VkExtent2D      swapChainExtent,
                         VkPipeline      graphicsPipeline,
                         VkFramebuffer   framebuffer) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = renderPass;
    renderPassInfo.framebuffer       = framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    VkClearValue clearColor        = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues    = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

/**
 * @brief 绘制一帧
 * 
 * 执行完整的帧渲染流程，包括获取图像、提交命令缓冲和呈现图像
 * 
 * @param device 逻辑设备
 * @param swapChain 交换链
 * @param graphicsQueue 图形队列
 * @param presentQueue 呈现队列
 * @param commandBuffers 命令缓冲集合
 * @param imageAvailableSemaphore 图像可用信号量
 * @param renderFinishedSemaphore 渲染完成信号量
 */
void drawFrame(VkDevice                            device,
               VkSwapchainKHR                      swapChain,
               VkQueue                             graphicsQueue,
               VkQueue                             presentQueue,
               const std::vector<VkCommandBuffer> &commandBuffers,
               VkSemaphore                         imageAvailableSemaphore,
               VkSemaphore                         renderFinishedSemaphore) {
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device,
                                            swapChain,
                                            UINT64_MAX,
                                            imageAvailableSemaphore,
                                            VK_NULL_HANDLE,
                                            &imageIndex);

    // 如果交换链已经过期（典型场景：窗口 resize），这里不再抛异常，而是交给外层重建
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        // 外层（Application::mainLoop 或 drawFrame 的调用者）应在合适时机调用 recreateSwapChain()
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore          waitSemaphores[] = {imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[]     = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount         = 1;
    submitInfo.pWaitSemaphores            = waitSemaphores;
    submitInfo.pWaitDstStageMask          = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffers[imageIndex];

    VkSemaphore signalSemaphores[]  = {renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount  = 1;
    presentInfo.pSwapchains     = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);
    // 如果呈现时发现交换链过期/次优，也交给外层去重建
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // 这里不抛异常，调用者在下一帧可以检查窗口 resize 状态并调用 recreateSwapChain()
        // 例如在 Application::mainLoop 中检测到 framebufferResized 或记录一个标志位
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    // demo 简化：等待呈现队列空闲，真实项目中一般会做更细粒度的同步
    vkQueueWaitIdle(presentQueue);
}
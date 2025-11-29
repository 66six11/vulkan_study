#pragma once

#include "platform/Platform.h"
#include "core/constants.h"
#include <vector>

/**
 * @brief 命令缓冲和同步相关函数命名空间
 * 
 * 包含命令池、命令缓冲、信号量创建以及帧绘制等函数
 */
namespace vkcmd {

/**
 * @brief 创建命令池
 * 
 * 创建命令池对象，用于分配命令缓冲，管理命令缓冲的内存
 * 
 * @param device 逻辑设备
 * @param indices 队列族索引
 * @param commandPool [out] 创建的命令池对象
 */
void createCommandPool(VkDevice device, QueueFamilyIndices indices, VkCommandPool& commandPool);

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
                          const std::vector<VkFramebuffer>& swapChainFramebuffers,
                          VkRenderPass                      renderPass,
                          VkExtent2D                        swapChainExtent,
                          VkPipeline                        graphicsPipeline,
                          const std::vector<VkImageView>&   swapChainImageViews,
                          std::vector<VkCommandBuffer>&     commandBuffers);

/**
 * @brief 创建信号量
 * 
 * 创建用于同步操作的信号量对象
 * 
 * @param device 逻辑设备
 * @param imageAvailableSemaphore [out] 图像可用信号量
 * @param renderFinishedSemaphore [out] 渲染完成信号量
 */
void createSemaphores(VkDevice device, VkSemaphore& imageAvailableSemaphore, VkSemaphore& renderFinishedSemaphore);

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
                         VkFramebuffer   framebuffer);

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
               const std::vector<VkCommandBuffer>& commandBuffers,
               VkSemaphore                         imageAvailableSemaphore,
               VkSemaphore                         renderFinishedSemaphore);

} // namespace vkcmd

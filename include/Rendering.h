#pragma once


#include "constants.h"
#include <vector>
#include <string>

/**
 * @brief 创建渲染通道
 * 
 * 创建渲染通道对象，定义渲染操作的附件和子通道，描述完整的渲染流程
 * 
 * @param device 逻辑设备
 * @param swapChainImageFormat 交换链图像格式
 * @param renderPass [out] 创建的渲染通道对象
 */
void createRenderPass(VkDevice device, VkFormat swapChainImageFormat, VkRenderPass& renderPass);

/**
 * @brief 创建图形管线
 * 
 * 创建图形管线对象，定义图形渲染的完整状态，包括顶点输入、装配、光栅化、片段处理等阶段
 * 
 * @param device 逻辑设备
 * @param swapChainExtent 交换链图像尺寸
 * @param renderPass 渲染通道
 * @param pipelineLayout [out] 管线布局
 * @param graphicsPipeline [out] 图形管线
 */
void createGraphicsPipeline(VkDevice          device,
                            VkExtent2D        swapChainExtent,
                            VkRenderPass      renderPass,
                            VkPipelineLayout& pipelineLayout,
                            VkPipeline&       graphicsPipeline);

/**
 * @brief 创建帧缓冲
 * 
 * 为每个交换链图像视图创建对应的帧缓冲对象，帧缓冲用于存储渲染附件
 * 
 * @param device 逻辑设备
 * @param swapChainImageViews 交换链图像视图集合
 * @param renderPass 渲染通道
 * @param swapChainExtent 交换链图像尺寸
 * @param swapChainFramebuffers [out] 创建的帧缓冲集合
 */
void createFramebuffers(VkDevice                        device,
                        const std::vector<VkImageView>& swapChainImageViews,
                        VkRenderPass                    renderPass,
                        VkExtent2D                      swapChainExtent,
                        std::vector<VkFramebuffer>&     swapChainFramebuffers);

/**
 * @brief 创建着色器模块
 * 
 * 从SPIR-V字节码创建着色器模块对象
 * 
 * @param device 逻辑设备
 * @param code 着色器代码（SPIR-V字节码）
 * @return 创建的着色器模块对象
 */
VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code);

#include "../include/HelloTriangleApplication.h"
#include "../include/rendering.h"
#include <stdexcept>

// 创建渲染通道
// 定义渲染操作的附件、子通道和依赖关系
void HelloTriangleApplication::createRenderPass() {
    // 创建颜色附件描述
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;                        // 附件格式
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;                     // 采样数量（不使用多重采样）
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;                // 加载操作（清除附件内容）
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;              // 存储操作（存储渲染结果）
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;     // 模板加载操作（不关心）
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;   // 模板存储操作（不关心）
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;           // 初始布局（未定义）
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;       // 最终布局（用于呈现）

    // 创建颜色附件引用
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;                                   // 附件索引
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // 附件布局

    // 创建子通道描述
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;         // 管线绑定点（图形管线）
    subpass.colorAttachmentCount = 1;                                    // 颜色附件数量
    subpass.pColorAttachments = &colorAttachmentRef;                     // 指向颜色附件引用

    // 子通道依赖，确保渲染在图像可用后进行
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;                         // 源子通道（外部）
    dependency.dstSubpass = 0;                                           // 目标子通道（第一个）
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;  // 源阶段掩码
    dependency.srcAccessMask = 0;                                        // 源访问掩码
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;  // 目标阶段掩码
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;     // 目标访问掩码

    // 创建渲染通道信息
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;    // 结构体类型标识
    renderPassInfo.attachmentCount = 1;                                  // 附件数量
    renderPassInfo.pAttachments = &colorAttachment;                      // 指向附件描述
    renderPassInfo.subpassCount = 1;                                     // 子通道数量
    renderPassInfo.pSubpasses = &subpass;                                // 指向子通道描述
    renderPassInfo.dependencyCount = 1;                                  // 依赖数量
    renderPassInfo.pDependencies = &dependency;                          // 指向依赖描述

    // 创建渲染通道
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

// 创建图形管线
// 创建图形管线布局（在这个基础示例中没有完整的图形管线）
void HelloTriangleApplication::createGraphicsPipeline() {
    // 由于没有实际的着色器文件，我们创建一个基础的图形管线布局
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;  // 结构体类型标识
    pipelineLayoutInfo.setLayoutCount = 0;                                     // 描述符集布局数量
    pipelineLayoutInfo.pSetLayouts = nullptr;                                  // 指向描述符集布局
    pipelineLayoutInfo.pushConstantRangeCount = 0;                             // 推送常量范围数量
    pipelineLayoutInfo.pPushConstantRanges = nullptr;                          // 指向推送常量范围

    // 创建管线布局
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }
    
    // 对于这个基础示例，我们只创建管线布局，不创建完整的图形管线
    // 在实际应用中，这里需要创建顶点着色器、片段着色器、管线等
    // 由于缺少着色器，我们暂时只创建布局
    
    // 在实际应用中，这里应该包含完整的图形管线创建代码
    // 包括着色器模块、顶点输入状态、输入装配状态、视口状态、光栅化状态、
    // 多重采样状态、深度模板状态、颜色混合状态等
}

// 创建帧缓冲
// 为每个交换链图像创建帧缓冲，用于存储渲染附件
void HelloTriangleApplication::createFramebuffers() {
    // 为每个交换链图像创建帧缓冲
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        // 定义附件数组
        VkImageView attachments[] = {swapChainImageViews[i]};

        // 填充帧缓冲创建信息
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;  // 结构体类型标识
        framebufferInfo.renderPass = renderPass;                            // 渲染通道
        framebufferInfo.attachmentCount = 1;                                // 附件数量
        framebufferInfo.pAttachments = attachments;                         // 指向附件数组
        framebufferInfo.width = swapChainExtent.width;                      // 帧缓冲宽度
        framebufferInfo.height = swapChainExtent.height;                    // 帧缓冲高度
        framebufferInfo.layers = 1;                                         // 层数

        // 创建帧缓冲
        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}
#include "vulkan_backend/Rendering.h"
#include <fstream>
#include <stdexcept>
#include <vector>
#include "vulkan_backend/VertexInputDescription.h"

namespace
{
    // 动态状态配置 - 允许在命令缓冲录制时动态修改这些管线状态
    constexpr std::array<VkDynamicState, 4> kDynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH,
        VK_DYNAMIC_STATE_DEPTH_BIAS
    };

    /**
     * @brief 读取文件内容
     * 
     * 从指定文件路径读取二进制内容到字符向量中
     */
    std::vector<char> readFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open())
        {
            throw std::runtime_error("failed to open file!");
        }

        size_t            fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }
} // anonymous namespace

namespace vkpipeline
{
    void createRenderPass(VkDevice device, VkFormat swapChainImageFormat, VkRenderPass& renderPass)
    {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format         = swapChainImageFormat;
        colorAttachment.samples        = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments    = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass    = 0;
        dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments    = &colorAttachment;
        renderPassInfo.subpassCount    = 1;
        renderPassInfo.pSubpasses      = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies   = &dependency;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void createGraphicsPipeline(
        VkDevice          device,
        VkExtent2D        swapChainExtent,
        VkRenderPass      renderPass,
        VkPipelineLayout& pipelineLayout,
        VkPipeline&       graphicsPipeline)
    {
        auto vertShaderCode = readFile("shaders/shader.vert.spv");
        auto fragShaderCode = readFile("shaders/shader.frag.spv");

        VkShaderModule vertShaderModule = createShaderModule(device, vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(device, fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName  = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName  = "main";


        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        // 顶点输入状态
        VkVertexInputBindingDescription      bindingDesc = vkvertex::getBindingDescription();
        auto                                 attrDescs   = vkvertex::getAttributeDescriptions();
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount   = 1;
        vertexInputInfo.pVertexBindingDescriptions      = &bindingDesc;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attrDescs.size());
        vertexInputInfo.pVertexAttributeDescriptions    = attrDescs.data();
        // 输入装配状态
        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO; // 输入装配状态
        inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;                         // 三角形列表
        inputAssembly.primitiveRestartEnable = VK_FALSE;                                                    // 不启用基元重启
        // 视口和裁剪状态
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO; // 视口和裁剪状态
        viewportState.viewportCount = 1;                                                     // 一个视口
        viewportState.pViewports    = nullptr;                                               // 使用动态视口
        viewportState.scissorCount  = 1;                                                     // 一个裁剪矩形
        viewportState.pScissors     = nullptr;                                               // 使用动态裁剪矩形
        //  光栅化状态
        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO; // 光栅化状态
        rasterizer.depthClampEnable        = VK_FALSE;                                                   // 不启用深度裁剪
        rasterizer.rasterizerDiscardEnable = VK_FALSE;                                                   // 不丢弃几何体
        rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;                                       // 填充多边形
        rasterizer.lineWidth               = 1.0f;                                                       // 线宽
        rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;                                      // 背面剔除
        rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;                                    // 顺时针为前面
        rasterizer.depthBiasEnable         = VK_FALSE;                                                   // 不启用深度偏移

        // 多重采样状态
        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable  = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        // 颜色混合状态
        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                              VK_COLOR_COMPONENT_G_BIT |
                                              VK_COLOR_COMPONENT_B_BIT |
                                              VK_COLOR_COMPONENT_A_BIT; // 写入所有颜色通道
        colorBlendAttachment.blendEnable = VK_FALSE;                    // 不启用混合
        // 整体颜色混合状态
        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType             = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO; // 颜色混合状态
        colorBlending.logicOpEnable     = VK_FALSE;                                                 // 不启用逻辑操作
        colorBlending.logicOp           = VK_LOGIC_OP_COPY;                                         // 逻辑操作（未启用时忽略）
        colorBlending.attachmentCount   = 1;                                                        // 一个颜色附件
        colorBlending.pAttachments      = &colorBlendAttachment;                                    // 颜色混合附件
        colorBlending.blendConstants[0] = 0.0f;                                                     // 混合常量
        colorBlending.blendConstants[1] = 0.0f;                                                     // 混合常量
        colorBlending.blendConstants[2] = 0.0f;                                                     // 混合常量
        colorBlending.blendConstants[3] = 0.0f;                                                     // 混合常量
        // 管线布局
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO; // 管线布局
        pipelineLayoutInfo.setLayoutCount         = 0;                                             // 不使用描述符集布局
        pipelineLayoutInfo.pushConstantRangeCount = 0;                                             // 不使用推送常量

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create pipeline layout!");
        }
        // 动态状态配置
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO; // 动态状态配置
        dynamicState.dynamicStateCount = static_cast<uint32_t>(kDynamicStates.size());         // 动态状态数量
        dynamicState.pDynamicStates    = kDynamicStates.data();                                // 动态状态数组
        // 最终管线创建信息
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO; // 图形管线创建信息
        pipelineInfo.stageCount          = 2;                                               // 着色器阶段数量
        pipelineInfo.pStages             = shaderStages;                                    // 着色器阶段数组
        pipelineInfo.pVertexInputState   = &vertexInputInfo;                                // 顶点输入状态
        pipelineInfo.pInputAssemblyState = &inputAssembly;                                  // 输入装配状态
        pipelineInfo.pViewportState      = &viewportState;                                  // 视口状态
        pipelineInfo.pRasterizationState = &rasterizer;                                     // 光栅化状态
        pipelineInfo.pMultisampleState   = &multisampling;                                  // 多重采样状态
        pipelineInfo.pColorBlendState    = &colorBlending;                                  // 颜色混合状态
        pipelineInfo.layout              = pipelineLayout;                                  // 管线布局
        pipelineInfo.renderPass          = renderPass;                                      // 渲染通道
        pipelineInfo.subpass             = 0;                                               // 子通道索引
        pipelineInfo.basePipelineHandle  = VK_NULL_HANDLE;                                  // 不使用派生管线
        pipelineInfo.pDynamicState       = &dynamicState;                                   // 动态状态信息


        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) !=
            VK_SUCCESS)
        {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }

    void createFramebuffers(
        VkDevice                        device,
        const std::vector<VkImageView>& swapChainImageViews,
        VkRenderPass                    renderPass,
        VkExtent2D                      swapChainExtent,
        std::vector<VkFramebuffer>&     swapChainFramebuffers)
    {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++)
        {
            VkImageView attachments[] = {
                swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass      = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments    = attachments;
            framebufferInfo.width           = swapChainExtent.width;
            framebufferInfo.height          = swapChainExtent.height;
            framebufferInfo.layers          = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    VkShaderModule createShaderModule(VkDevice device, const std::vector<char>& code)
    {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode    = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }
} // namespace vkpipeline

#pragma once

// Compatibility layer: Pipeline

#include "engine/rhi/Pipeline.hpp"

namespace engine::vulkan
{
    // Pipeline types are now aliases to engine::rhi
    using GraphicsPipeline          = rhi::GraphicsPipeline;
    using GraphicsPipelineHandle    = rhi::GraphicsPipelineHandle;
    using ComputePipeline           = rhi::ComputePipeline;
    using ComputePipelineHandle     = rhi::ComputePipelineHandle;
    using Shader                    = rhi::Shader;
    using ShaderHandle              = rhi::ShaderHandle;
    using DescriptorSetLayout       = rhi::DescriptorSetLayout;
    using DescriptorSetLayoutHandle = rhi::DescriptorSetLayoutHandle;
    using PipelineLayout            = rhi::PipelineLayout;
    using PipelineLayoutHandle      = rhi::PipelineLayoutHandle;
} // namespace engine::vulkan
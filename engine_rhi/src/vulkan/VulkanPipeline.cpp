#include <string>

#include "engine/rhi/Pipeline.hpp"
#include <vulkan/vulkan.h>

namespace engine::rhi
{
    // Shader implementation
    Shader::~Shader()
    {
        release();
    }

    Shader::Shader(Shader&& other) noexcept
        : handle_(other.handle_)
        , device_(other.device_)
        , stage_(other.stage_)
        , entryPoint_(std::move(other.entryPoint_))
    {
        other.handle_ = nullptr;
        other.device_ = nullptr;
    }

    Shader& Shader::operator=(Shader&& other) noexcept
    {
        if (this != &other)
        {
            release();
            handle_       = other.handle_;
            device_       = other.device_;
            stage_        = other.stage_;
            entryPoint_   = std::move(other.entryPoint_);
            other.handle_ = nullptr;
            other.device_ = nullptr;
        }
        return *this;
    }

    Shader::Shader(const InternalData& data, ShaderStage stage, std::string entryPoint)
        : handle_(data.shader)
        , device_(data.device)
        , stage_(stage)
        , entryPoint_(std::move(entryPoint))
    {
    }

    void Shader::release()
    {
        if (handle_ && device_)
        {
            vkDestroyShaderModule(device_, handle_, nullptr);
            handle_ = nullptr;
            device_ = nullptr;
        }
    }

    // DescriptorSetLayout implementation
    DescriptorSetLayout::~DescriptorSetLayout()
    {
        release();
    }

    DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other) noexcept
        : handle_(other.handle_)
        , device_(other.device_)
        , desc_(other.desc_)
    {
        other.handle_ = nullptr;
        other.device_ = nullptr;
    }

    DescriptorSetLayout& DescriptorSetLayout::operator=(DescriptorSetLayout&& other) noexcept
    {
        if (this != &other)
        {
            release();
            handle_       = other.handle_;
            device_       = other.device_;
            desc_         = other.desc_;
            other.handle_ = nullptr;
            other.device_ = nullptr;
        }
        return *this;
    }

    DescriptorSetLayout::DescriptorSetLayout(const InternalData& data, const DescriptorSetLayoutDesc& desc)
        : handle_(data.layout)
        , device_(data.device)
        , desc_(desc)
    {
    }

    void DescriptorSetLayout::release()
    {
        if (handle_ && device_)
        {
            vkDestroyDescriptorSetLayout(device_, handle_, nullptr);
            handle_ = nullptr;
            device_ = nullptr;
        }
    }

    // PipelineLayout implementation
    PipelineLayout::~PipelineLayout()
    {
        release();
    }

    PipelineLayout::PipelineLayout(PipelineLayout&& other) noexcept
        : handle_(other.handle_)
        , device_(other.device_)
        , desc_(other.desc_)
    {
        other.handle_ = nullptr;
        other.device_ = nullptr;
    }

    PipelineLayout& PipelineLayout::operator=(PipelineLayout&& other) noexcept
    {
        if (this != &other)
        {
            release();
            handle_       = other.handle_;
            device_       = other.device_;
            desc_         = other.desc_;
            other.handle_ = nullptr;
            other.device_ = nullptr;
        }
        return *this;
    }

    PipelineLayout::PipelineLayout(const InternalData& data, const PipelineLayoutDesc& desc)
        : handle_(data.layout)
        , device_(data.device)
        , desc_(desc)
    {
    }

    void PipelineLayout::release()
    {
        if (handle_ && device_)
        {
            vkDestroyPipelineLayout(device_, handle_, nullptr);
            handle_ = nullptr;
            device_ = nullptr;
        }
    }

    // GraphicsPipeline implementation
    GraphicsPipeline::~GraphicsPipeline()
    {
        release();
    }

    GraphicsPipeline::GraphicsPipeline(GraphicsPipeline&& other) noexcept
        : handle_(other.handle_)
        , device_(other.device_)
        , layout_(std::move(other.layout_))
    {
        other.handle_ = nullptr;
        other.device_ = nullptr;
    }

    GraphicsPipeline& GraphicsPipeline::operator=(GraphicsPipeline&& other) noexcept
    {
        if (this != &other)
        {
            release();
            handle_       = other.handle_;
            device_       = other.device_;
            layout_       = std::move(other.layout_);
            other.handle_ = nullptr;
            other.device_ = nullptr;
        }
        return *this;
    }

    GraphicsPipeline::GraphicsPipeline(const InternalData& data)
        : handle_(data.pipeline)
        , device_(data.device)
        , layout_(data.layout)
    {
    }

    void GraphicsPipeline::release()
    {
        if (handle_ && device_)
        {
            vkDestroyPipeline(device_, handle_, nullptr);
            handle_ = nullptr;
            device_ = nullptr;
            layout_.reset();
        }
    }

    // ComputePipeline implementation
    ComputePipeline::~ComputePipeline()
    {
        release();
    }

    ComputePipeline::ComputePipeline(ComputePipeline&& other) noexcept
        : handle_(other.handle_)
        , device_(other.device_)
        , layout_(std::move(other.layout_))
    {
        other.handle_ = nullptr;
        other.device_ = nullptr;
    }

    ComputePipeline& ComputePipeline::operator=(ComputePipeline&& other) noexcept
    {
        if (this != &other)
        {
            release();
            handle_       = other.handle_;
            device_       = other.device_;
            layout_       = std::move(other.layout_);
            other.handle_ = nullptr;
            other.device_ = nullptr;
        }
        return *this;
    }

    ComputePipeline::ComputePipeline(const InternalData& data)
        : handle_(data.pipeline)
        , device_(data.device)
        , layout_(data.layout)
    {
    }

    void ComputePipeline::release()
    {
        if (handle_ && device_)
        {
            vkDestroyPipeline(device_, handle_, nullptr);
            handle_ = nullptr;
            device_ = nullptr;
            layout_.reset();
        }
    }
} // namespace engine::rhi

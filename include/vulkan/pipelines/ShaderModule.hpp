#pragma once

#include "vulkan/device/Device.hpp"
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <memory>

namespace vulkan_engine::vulkan
{
    // Shader module wrapper with RAII
    class ShaderModule
    {
        public:
            // Create from SPIR-V bytecode
            ShaderModule(
                std::shared_ptr<DeviceManager> device,
                const std::vector<uint32_t>&   code);

            // Create from raw bytes (will copy and align)
            ShaderModule(
                std::shared_ptr<DeviceManager> device,
                const uint8_t*                 data,
                size_t                         size);

            // Create from file path
            ShaderModule(
                std::shared_ptr<DeviceManager> device,
                const std::string&             file_path);

            ~ShaderModule();

            // Non-copyable
            ShaderModule(const ShaderModule&)            = delete;
            ShaderModule& operator=(const ShaderModule&) = delete;

            // Movable
            ShaderModule(ShaderModule&& other) noexcept;
            ShaderModule& operator=(ShaderModule&& other) noexcept;

            // Accessors
            VkShaderModule handle() const { return module_; }
            bool           valid() const { return module_ != VK_NULL_HANDLE; }

            // Utility functions
            static std::vector<uint32_t> load_spirv_from_file(const std::string& path);

        private:
            std::shared_ptr<DeviceManager> device_;
            VkShaderModule                 module_ = VK_NULL_HANDLE;
    };

    // Shader stage configuration helper
    struct ShaderStageInfo
    {
        VkShaderStageFlagBits       stage;
        VkShaderModule              module;
        const char*                 entry_point    = "main";
        const VkSpecializationInfo* specialization = nullptr;

        VkPipelineShaderStageCreateInfo create_info() const
        {
            VkPipelineShaderStageCreateInfo info{};
            info.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            info.stage               = stage;
            info.module              = module;
            info.pName               = entry_point;
            info.pSpecializationInfo = specialization;
            return info;
        }
    };
} // namespace vulkan_engine::vulkan
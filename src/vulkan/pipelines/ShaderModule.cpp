#include "vulkan/pipelines/ShaderModule.hpp"
#include "vulkan/utils/VulkanError.hpp"
#include <fstream>
#include <cstring>

namespace vulkan_engine::vulkan
{
    // Create from SPIR-V bytecode (aligned uint32_t array)
    ShaderModule::ShaderModule(
        std::shared_ptr<DeviceManager> device,
        const std::vector<uint32_t>&   code)
        : device_(std::move(device))
    {
        if (code.empty())
        {
            throw std::runtime_error("Cannot create shader module from empty code");
        }

        VkShaderModuleCreateInfo create_info{};
        create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = code.size() * sizeof(uint32_t);
        create_info.pCode    = code.data();

        VkResult result = vkCreateShaderModule(device_->device(), &create_info, nullptr, &module_);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create shader module from code", __FILE__, __LINE__);
        }
    }

    // Create from raw bytes (handles unaligned data)
    ShaderModule::ShaderModule(
        std::shared_ptr<DeviceManager> device,
        const uint8_t*                 data,
        size_t                         size)
        : device_(std::move(device))
    {
        if (!data || size == 0)
        {
            throw std::runtime_error("Cannot create shader module from empty data");
        }

        // SPIR-V code must be 4-byte aligned
        // Copy to aligned buffer
        size_t                code_size = (size + 3) / 4; // Round up to uint32 count
        std::vector<uint32_t> code(code_size);
        std::memcpy(code.data(), data, size);

        VkShaderModuleCreateInfo create_info{};
        create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = size; // Original size, not padded
        create_info.pCode    = code.data();

        VkResult result = vkCreateShaderModule(device_->device(), &create_info, nullptr, &module_);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create shader module from bytes", __FILE__, __LINE__);
        }
    }

    // Create from file path
    ShaderModule::ShaderModule(
        std::shared_ptr<DeviceManager> device,
        const std::string&             file_path)
        : device_(std::move(device))
    {
        auto code = load_spirv_from_file(file_path);

        VkShaderModuleCreateInfo create_info{};
        create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        create_info.codeSize = code.size() * sizeof(uint32_t);
        create_info.pCode    = code.data();

        VkResult result = vkCreateShaderModule(device_->device(), &create_info, nullptr, &module_);
        if (result != VK_SUCCESS)
        {
            throw VulkanError(result, "Failed to create shader module from file: " + file_path, __FILE__, __LINE__);
        }
    }

    ShaderModule::~ShaderModule()
    {
        if (module_ != VK_NULL_HANDLE && device_)
        {
            vkDestroyShaderModule(device_->device(), module_, nullptr);
        }
    }

    ShaderModule::ShaderModule(ShaderModule&& other) noexcept
        : device_(std::move(other.device_))
        , module_(other.module_)
    {
        other.module_ = VK_NULL_HANDLE;
    }

    ShaderModule& ShaderModule::operator=(ShaderModule&& other) noexcept
    {
        if (this != &other)
        {
            if (module_ != VK_NULL_HANDLE && device_)
            {
                vkDestroyShaderModule(device_->device(), module_, nullptr);
            }

            device_       = std::move(other.device_);
            module_       = other.module_;
            other.module_ = VK_NULL_HANDLE;
        }
        return *this;
    }

    std::vector<uint32_t> ShaderModule::load_spirv_from_file(const std::string& path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open SPIR-V file: " + path);
        }

        auto file_size = static_cast<size_t>(file.tellg());
        if (file_size == 0)
        {
            throw std::runtime_error("SPIR-V file is empty: " + path);
        }

        if (file_size % 4 != 0)
        {
            throw std::runtime_error("SPIR-V file size is not 4-byte aligned: " + path);
        }

        std::vector<uint32_t> buffer(file_size / sizeof(uint32_t));

        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), file_size);
        file.close();

        // Validate SPIR-V magic number
        if (buffer.empty() || buffer[0] != 0x07230203)
        {
            throw std::runtime_error("Invalid SPIR-V file (wrong magic number): " + path);
        }

        return buffer;
    }
} // namespace vulkan_engine::vulkan
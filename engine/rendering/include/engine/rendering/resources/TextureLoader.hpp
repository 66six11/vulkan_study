#pragma once

#include "engine/rhi/vulkan/resources/Image.hpp"
#include "engine/rhi/vulkan/device/Device.hpp"

#include <string>
#include <memory>

namespace vulkan_engine::rendering
{
    // ============================================================================
    // TextureLoader - Loads texture images from files
    // ============================================================================
    class TextureLoader
    {
        public:
            explicit TextureLoader(std::shared_ptr<vulkan::DeviceManager> device);
            ~TextureLoader() = default;

            // Load texture from file (supports PNG, JPG, BMP, etc.)
            // Returns nullptr if loading fails
            std::shared_ptr<vulkan::Image> load_texture(
                const std::string& path,
                bool               generate_mipmaps = true);

            // Load texture with explicit format
            std::shared_ptr<vulkan::Image> load_texture(
                const std::string& path,
                VkFormat           format,
                bool               generate_mipmaps = true);

            // Set base directory for texture paths
            void set_base_directory(const std::string& path) { base_directory_ = path; }

            // Helper to resolve texture paths
            std::string resolve_path(const std::string& path) const;

        private:
            std::shared_ptr<vulkan::DeviceManager> device_;
            std::string                            base_directory_ = "textures/";

            // Create GPU image from raw pixel data
            std::shared_ptr<vulkan::Image> create_image_from_data(
                const void* pixel_data,
                uint32_t    width,
                uint32_t    height,
                uint32_t    channels,
                bool        generate_mipmaps);

            // Calculate mipmap levels
            uint32_t calculate_mip_levels(uint32_t width, uint32_t height) const;
    };
} // namespace vulkan_engine::rendering
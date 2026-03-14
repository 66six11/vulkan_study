#pragma once

#include "rendering/material/Material.hpp"
#include "rendering/resources/TextureLoader.hpp"

#include <string>
#include <memory>
#include <unordered_map>

namespace vulkan_engine::rendering
{
    // ============================================================================
    // MaterialLoader - Loads material definitions from JSON files
    // ============================================================================
    class MaterialLoader
    {
        public:
            explicit MaterialLoader(std::shared_ptr<vulkan::DeviceManager> device);
            ~MaterialLoader() = default;

            // Load a material from JSON file
            std::shared_ptr<Material> load(const std::string& path, VkRenderPass render_pass);

            // Get cached material
            std::shared_ptr<Material> get(const std::string& name) const;

            // Check if material is cached
            bool has(const std::string& name) const;

            // Clear cache
            void clear_cache();

            // Set materials base directory
            void set_base_directory(const std::string& path) { base_directory_ = path; }

            // Set textures base directory
            void set_texture_directory(const std::string& path) { texture_loader_.set_base_directory(path); }

        private:
            std::shared_ptr<vulkan::DeviceManager>                     device_;
            std::string                                                base_directory_ = "materials/";
            std::unordered_map<std::string, std::shared_ptr<Material>> material_cache_;
            TextureLoader                                              texture_loader_;

            // Parse JSON file
            Material::Config parse_json(const std::string& path, VkRenderPass render_pass);

            // Helper to resolve shader paths
            std::string resolve_path(const std::string& path) const;
    };
} // namespace vulkan_engine::rendering
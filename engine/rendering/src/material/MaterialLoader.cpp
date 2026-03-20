#include "rendering/material/MaterialLoader.hpp"
#include "core/utils/Logger.hpp"
#include "platform/filesystem/PathUtils.hpp"
#include "platform/filesystem/FileSystem.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

namespace vulkan_engine::rendering
{
    using json = nlohmann::json;

    // Helper to generate cache key that includes format information
    static std::string generate_cache_key(const std::string& name, VkFormat color_format, VkFormat depth_format)
    {
        return name + "_" + std::to_string(color_format) + "_" + std::to_string(depth_format);
    }

    // Helper to generate cache key for traditional render pass
    static std::string generate_cache_key(const std::string& name, VkRenderPass render_pass)
    {
        return name + "_rp_" + std::to_string(reinterpret_cast<uint64_t>(render_pass));
    }

    MaterialLoader::MaterialLoader(std::shared_ptr<vulkan::DeviceManager> device)
        : device_(std::move(device)), texture_loader_(device_)
    {
    }

    std::shared_ptr<Material> MaterialLoader::load(const std::string& path, VkRenderPass render_pass)
    {
        // 直接写死路径，不解析
        std::string full_path = "D:/TechArt/Vulkan/materials/" + path;

        // Parse JSON and create material
        try
        {
            // Read JSON file
            auto file = std::ifstream(full_path);
            if (!file.is_open())
            {
                logger::error("Failed to open material file: " + full_path);
                return nullptr;
            }

            json j;
            file >> j;
            file.close();

            // Parse config
            Material::Config config;
            config.render_pass = render_pass;
            config.name        = j.value("name", "DefaultMaterial");

            // Generate cache key that includes render pass
            std::string cache_key = generate_cache_key(config.name, render_pass);

            // Check cache using generated key
            auto it = material_cache_.find(cache_key);
            if (it != material_cache_.end())
            {
                logger::info("Material '" + config.name + "' (render pass) found in cache");
                return it->second;
            }

            // Parse shader paths
            if (j.contains("shader"))
            {
                config.vertex_shader_path   = j["shader"].value("vertex", "shaders/pbr.vert.spv");
                config.fragment_shader_path = j["shader"].value("fragment", "shaders/pbr.frag.spv");
            }
            else
            {
                // Default to PBR shaders
                config.vertex_shader_path   = "shaders/pbr.vert.spv";
                config.fragment_shader_path = "shaders/pbr.frag.spv";
            }

            // Resolve paths
            config.vertex_shader_path   = resolve_path(config.vertex_shader_path);
            config.fragment_shader_path = resolve_path(config.fragment_shader_path);

            // Parse render states
            if (j.contains("render_states"))
            {
                auto& rs           = j["render_states"];
                config.depth_test  = rs.value("depth_test", true);
                config.depth_write = rs.value("depth_write", true);
            }

            // Create material
            auto material = std::make_shared<Material>(device_, config);

            // Parse and set parameters
            if (j.contains("parameters"))
            {
                auto& params = j["parameters"];

                // Color
                if (params.contains("color"))
                {
                    auto& color = params["color"];
                    if (color.contains("value"))
                    {
                        auto& val = color["value"];
                        if (val.is_array() && val.size() >= 3)
                        {
                            glm::vec3 color_vec(val[0].get<float>(), val[1].get<float>(), val[2].get<float>());
                            material->set_vec3("color", color_vec);
                            logger::info("  Color: " + std::to_string(color_vec.r) + ", " +
                                         std::to_string(color_vec.g) + ", " + std::to_string(color_vec.b));
                        }
                    }
                }

                // Roughness
                if (params.contains("roughness"))
                {
                    float roughness = params["roughness"].value("value", 0.5f);
                    material->set_float("roughness", roughness);
                    logger::info("  Roughness: " + std::to_string(roughness));
                }

                // Metallic
                if (params.contains("metallic"))
                {
                    float metallic = params["metallic"].value("value", 0.0f);
                    material->set_float("metallic", metallic);
                    logger::info("  Metallic: " + std::to_string(metallic));
                }

                // Emissive
                if (params.contains("emissive"))
                {
                    float emissive = params["emissive"].value("value", 0.0f);
                    material->set_float("emissive", emissive);
                    logger::info("  Emissive: " + std::to_string(emissive));
                }
            }

            // Parse and load textures
            if (j.contains("textures"))
            {
                auto& textures = j["textures"];

                // Albedo/Diffuse texture (binding 1)
                if (textures.contains("albedo"))
                {
                    std::string texture_path = textures["albedo"].value("path", "");
                    if (!texture_path.empty())
                    {
                        auto texture = texture_loader_.load_texture(texture_path, true);
                        if (texture)
                        {
                            material->set_texture("albedo", texture, texture->view());
                            logger::info("  Albedo texture: " + texture_path);
                        }
                    }
                }

                // Normal map (binding 2)
                if (textures.contains("normal"))
                {
                    std::string texture_path = textures["normal"].value("path", "");
                    if (!texture_path.empty())
                    {
                        auto texture = texture_loader_.load_texture(texture_path, true);
                        if (texture)
                        {
                            material->set_texture("normal", texture, texture->view());
                            logger::info("  Normal texture: " + texture_path);
                        }
                    }
                }

                // Roughness map (binding 3)
                if (textures.contains("roughness"))
                {
                    std::string texture_path = textures["roughness"].value("path", "");
                    if (!texture_path.empty())
                    {
                        auto texture = texture_loader_.load_texture(texture_path, true);
                        if (texture)
                        {
                            material->set_texture("roughness", texture, texture->view());
                            logger::info("  Roughness texture: " + texture_path);
                        }
                    }
                }

                // Metallic map (binding 4)
                if (textures.contains("metallic"))
                {
                    std::string texture_path = textures["metallic"].value("path", "");
                    if (!texture_path.empty())
                    {
                        auto texture = texture_loader_.load_texture(texture_path, true);
                        if (texture)
                        {
                            material->set_texture("metallic", texture, texture->view());
                            logger::info("  Metallic texture: " + texture_path);
                        }
                    }
                }
            }

            // For traditional render pass version, we need to get formats from somewhere
            // This version is deprecated, use the color_format/depth_format version instead
            logger::warn("MaterialLoader::load(path, render_pass) is deprecated, use load(path, color_format, depth_format) instead");
            material->build(VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_D32_SFLOAT);

            // Cache the material using key that includes render pass
            material_cache_[cache_key] = material;

            logger::info("Material " + config.name + " loaded from " + full_path);

            return material;
        }
        catch (const std::exception& e)
        {
            logger::error("Failed to load material from " + full_path + ": " + e.what());
            return nullptr;
        }
    }

    std::shared_ptr<Material> MaterialLoader::load(const std::string& path, VkFormat color_format, VkFormat depth_format)
    {
        // 直接写死路径，不解析
        std::string full_path = "D:/TechArt/Vulkan/materials/" + path;

        // Parse JSON and create material
        try
        {
            // Read JSON file
            auto file = std::ifstream(full_path);
            if (!file.is_open())
            {
                logger::error("Failed to open material file: " + full_path);
                return nullptr;
            }

            json j;
            file >> j;
            file.close();

            // Parse config
            Material::Config config;
            config.color_format = color_format;
            config.depth_format = depth_format;
            config.name         = j.value("name", "DefaultMaterial");

            // Generate cache key that includes color and depth formats
            std::string cache_key = generate_cache_key(config.name, color_format, depth_format);

            // Check cache using generated key
            auto it = material_cache_.find(cache_key);
            if (it != material_cache_.end())
            {
                logger::info("Material '" + config.name + "' (" + std::to_string(color_format) + "/" + std::to_string(depth_format) +
                             ") found in cache");
                return it->second;
            }

            // Parse shader paths
            if (j.contains("shader"))
            {
                config.vertex_shader_path   = j["shader"].value("vertex", "shaders/pbr.vert.spv");
                config.fragment_shader_path = j["shader"].value("fragment", "shaders/pbr.frag.spv");
            }
            else
            {
                config.vertex_shader_path   = "shaders/pbr.vert.spv";
                config.fragment_shader_path = "shaders/pbr.frag.spv";
            }

            // Resolve paths
            config.vertex_shader_path   = resolve_path(config.vertex_shader_path);
            config.fragment_shader_path = resolve_path(config.fragment_shader_path);

            // Parse render states
            if (j.contains("render_states"))
            {
                auto& rs           = j["render_states"];
                config.depth_test  = rs.value("depth_test", true);
                config.depth_write = rs.value("depth_write", true);
            }

            // Create material
            auto material = std::make_shared<Material>(device_, config);

            // Parse and set parameters (reuse same logic)
            if (j.contains("parameters"))
            {
                auto& params = j["parameters"];

                if (params.contains("color"))
                {
                    auto& color = params["color"];
                    if (color.contains("value"))
                    {
                        auto& val = color["value"];
                        if (val.is_array() && val.size() >= 3)
                        {
                            glm::vec3 color_vec(val[0].get<float>(), val[1].get<float>(), val[2].get<float>());
                            material->set_vec3("color", color_vec);
                            logger::info("  Color: " + std::to_string(color_vec.r) + ", " +
                                         std::to_string(color_vec.g) + ", " + std::to_string(color_vec.b));
                        }
                    }
                }

                if (params.contains("roughness"))
                {
                    float roughness = params["roughness"].value("value", 0.5f);
                    material->set_float("roughness", roughness);
                    logger::info("  Roughness: " + std::to_string(roughness));
                }

                if (params.contains("metallic"))
                {
                    float metallic = params["metallic"].value("value", 0.0f);
                    material->set_float("metallic", metallic);
                    logger::info("  Metallic: " + std::to_string(metallic));
                }

                if (params.contains("emissive"))
                {
                    float emissive = params["emissive"].value("value", 0.0f);
                    material->set_float("emissive", emissive);
                    logger::info("  Emissive: " + std::to_string(emissive));
                }
            }

            // Parse and load textures
            if (j.contains("textures"))
            {
                auto& textures = j["textures"];

                if (textures.contains("albedo"))
                {
                    std::string texture_path = textures["albedo"].value("path", "");
                    if (!texture_path.empty())
                    {
                        auto texture = texture_loader_.load_texture(texture_path, true);
                        if (texture)
                        {
                            material->set_texture("albedo", texture, texture->view());
                            logger::info("  Albedo texture: " + texture_path);
                        }
                    }
                }
            }

            // Build for dynamic rendering
            material->build(color_format, depth_format);

            // Cache the material using key that includes formats
            material_cache_[cache_key] = material;

            logger::info("Material " + config.name + " loaded from " + full_path);
            return material;
        }
        catch (const std::exception& e)
        {
            logger::error("Failed to load material from " + full_path + ": " + e.what());
            return nullptr;
        }
    }

    std::shared_ptr<Material> MaterialLoader::get(const std::string& name) const
    {
        auto it = material_cache_.find(name);
        if (it != material_cache_.end())
        {
            return it->second;
        }
        return nullptr;
    }

    bool MaterialLoader::has(const std::string& name) const
    {
        return material_cache_.find(name) != material_cache_.end();
    }

    void MaterialLoader::clear_cache()
    {
        material_cache_.clear();
    }

    std::string MaterialLoader::resolve_path(const std::string& path) const
    {
        // First try PathUtils resolution
        auto resolved = core::PathUtils::resolve(path);
        if (std::filesystem::exists(resolved))
        {
            return resolved.string();
        }

        // Try multiple search paths relative to base directory
        std::vector<std::string> search_paths = {
            base_directory_ + path,
            path,
            "../" + path,
            "../../" + path,
            "../../../" + path,
        };

        for (const auto& search_path : search_paths)
        {
            auto file = core::PathUtils::open_input_file(search_path, std::ios::binary);
            if (file.is_open())
            {
                file.close();
                return search_path;
            }
        }

        // Return PathUtils resolved path even if not found
        // (let the caller handle the error)
        return resolved.string();
    }
} // namespace vulkan_engine::rendering
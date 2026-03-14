#include "rendering/material/MaterialLoader.hpp"
#include "core/utils/Logger.hpp"
#include "platform/filesystem/FileSystem.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

namespace vulkan_engine::rendering
{
    using json = nlohmann::json;

    MaterialLoader::MaterialLoader(std::shared_ptr<vulkan::DeviceManager> device)
        : device_(std::move(device)), texture_loader_(device_)
    {
    }

    std::shared_ptr<Material> MaterialLoader::load(const std::string& path, VkRenderPass render_pass)
    {
        std::string full_path = base_directory_ + path;

        // Parse JSON and create material
        try
        {
            // Read JSON file
            std::ifstream file(full_path);
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

            // Check cache using material name (not full_path)
            auto it = material_cache_.find(config.name);
            if (it != material_cache_.end())
            {
                logger::info("Material '" + config.name + "' found in cache");
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

            material->build();

            // Cache the material using name as key (consistent with get() and has())
            material_cache_[config.name] = material;

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
        // Try multiple search paths
        std::vector<std::string> search_paths = {
            path,
            "../" + path,
            "../../" + path,
            "../../../" + path,
            "D:/TechArt/Vulkan/" + path
        };

        for (const auto& search_path : search_paths)
        {
            std::ifstream file(search_path, std::ios::binary);
            if (file.is_open())
            {
                file.close();
                return search_path;
            }
        }

        // Return original path if not found (will fail later with better error message)
        return path;
    }
} // namespace vulkan_engine::rendering
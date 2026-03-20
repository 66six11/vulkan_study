#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <queue>
#include <mutex>

namespace vulkan_engine::rendering
{
    using ResourceID                                = uint32_t;
    static constexpr ResourceID INVALID_RESOURCE_ID = 0;

    enum class ShaderType
    {
        Vertex,
        Fragment,
        Geometry,
        Compute,
        TessellationControl,
        TessellationEvaluation
    };

    struct Mesh
    {
        ResourceID  id = INVALID_RESOURCE_ID;
        std::string name;
        // Vertex data, indices, etc.
    };

    struct Texture
    {
        ResourceID  id = INVALID_RESOURCE_ID;
        std::string name;
        uint32_t    width  = 0;
        uint32_t    height = 0;
        // Vulkan image, etc.
    };

    struct MaterialResource
    {
        ResourceID  id = INVALID_RESOURCE_ID;
        std::string name;
        // Shader, textures, parameters
    };

    struct Shader
    {
        ResourceID  id = INVALID_RESOURCE_ID;
        std::string name;
        ShaderType  type;
        // Vulkan shader module
    };

    struct MeshLoadOptions
    {
        bool calculate_normals  = true;
        bool calculate_tangents = false;
        bool flip_uvs           = false;
    };

    struct TextureLoadOptions
    {
        bool generate_mipmaps = true;
        bool srgb             = false;
        bool flip_y           = false;
    };

    struct MaterialDesc
    {
        std::string name;
        // Shader references, texture references, parameters
    };

    using MeshLoader    = std::function<Mesh(const std::filesystem::path &, const MeshLoadOptions &)>;
    using TextureLoader = std::function<Texture(const std::filesystem::path &, const TextureLoadOptions &)>;

    class ResourceManager
    {
        public:
            ResourceManager();
            ~ResourceManager();

            // Mesh loading
            ResourceID load_mesh(const std::filesystem::path& path, const MeshLoadOptions& options = {});
            void       unload_mesh(ResourceID id);
            Mesh*      get_mesh(ResourceID id);
            bool       is_mesh_loaded(ResourceID id) const;

            // Texture loading
            ResourceID load_texture(const std::filesystem::path& path, const TextureLoadOptions& options = {});
            void       unload_texture(ResourceID id);
            Texture*   get_texture(ResourceID id);
            bool       is_texture_loaded(ResourceID id) const;

            // Material management
            ResourceID        create_material(const MaterialDesc& desc);
            void              unload_material(ResourceID id);
            MaterialResource* get_material(ResourceID id);
            bool              is_material_loaded(ResourceID id) const;

            // Shader loading
            ResourceID load_shader(const std::filesystem::path& path, ShaderType type);
            void       unload_shader(ResourceID id);
            Shader*    get_shader(ResourceID id);
            bool       is_shader_loaded(ResourceID id) const;

            // Async loading
            void load_async(std::function<void()> load_func);

            void wait_for_all_loads();

            // Hot reload
            void update_hot_reloads();
            void enable_hot_reloading(bool enable);

            // Custom loaders
            void register_mesh_loader(const std::string& extension, MeshLoader loader);
            void register_texture_loader(const std::string& extension, TextureLoader loader);

        private:
            struct Impl;
            std::unique_ptr<Impl> impl_;
    };
} // namespace vulkan_engine::rendering
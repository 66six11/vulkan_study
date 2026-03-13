#include "rendering/resources/ResourceManager.hpp"
#include <stdexcept>

namespace vulkan_engine::rendering
{
    struct ResourceManager::Impl
    {
        // Resource storage
        std::unordered_map<ResourceID, Mesh>     meshes;
        std::unordered_map<ResourceID, Texture>  textures;
        std::unordered_map<ResourceID, Material> materials;
        std::unordered_map<ResourceID, Shader>   shaders;

        ResourceID next_id = 1;

        // Async loading queue
        std::queue<std::function<void()>> load_queue;
        std::mutex                        queue_mutex;
    };

    ResourceManager::ResourceManager()
        : impl_(std::make_unique<Impl>())
    {
    }

    ResourceManager::~ResourceManager() = default;

    ResourceID ResourceManager::load_mesh(const std::filesystem::path& path, const MeshLoadOptions& options)
    {
        auto id = impl_->next_id++;
        // Placeholder: Actual mesh loading implementation would go here
        Mesh mesh;
        mesh.id           = id;
        mesh.name         = path.filename().string();
        impl_->meshes[id] = std::move(mesh);
        return id;
    }

    ResourceID ResourceManager::load_texture(const std::filesystem::path& path, const TextureLoadOptions& options)
    {
        auto id = impl_->next_id++;
        // Placeholder: Actual texture loading implementation would go here
        Texture texture;
        texture.id          = id;
        texture.name        = path.filename().string();
        impl_->textures[id] = std::move(texture);
        return id;
    }

    ResourceID ResourceManager::create_material(const MaterialDesc& desc)
    {
        auto     id = impl_->next_id++;
        Material material;
        material.id          = id;
        material.name        = desc.name;
        impl_->materials[id] = std::move(material);
        return id;
    }

    ResourceID ResourceManager::load_shader(const std::filesystem::path& path, ShaderType type)
    {
        auto id = impl_->next_id++;
        // Placeholder: Actual shader loading implementation would go here
        Shader shader;
        shader.id          = id;
        shader.name        = path.filename().string();
        shader.type        = type;
        impl_->shaders[id] = std::move(shader);
        return id;
    }

    void ResourceManager::unload_mesh(ResourceID id)
    {
        impl_->meshes.erase(id);
    }

    void ResourceManager::unload_texture(ResourceID id)
    {
        impl_->textures.erase(id);
    }

    void ResourceManager::unload_material(ResourceID id)
    {
        impl_->materials.erase(id);
    }

    void ResourceManager::unload_shader(ResourceID id)
    {
        impl_->shaders.erase(id);
    }

    Mesh* ResourceManager::get_mesh(ResourceID id)
    {
        auto it = impl_->meshes.find(id);
        return (it != impl_->meshes.end()) ? &it->second : nullptr;
    }

    Texture* ResourceManager::get_texture(ResourceID id)
    {
        auto it = impl_->textures.find(id);
        return (it != impl_->textures.end()) ? &it->second : nullptr;
    }

    Material* ResourceManager::get_material(ResourceID id)
    {
        auto it = impl_->materials.find(id);
        return (it != impl_->materials.end()) ? &it->second : nullptr;
    }

    Shader* ResourceManager::get_shader(ResourceID id)
    {
        auto it = impl_->shaders.find(id);
        return (it != impl_->shaders.end()) ? &it->second : nullptr;
    }

    bool ResourceManager::is_mesh_loaded(ResourceID id) const
    {
        return impl_->meshes.find(id) != impl_->meshes.end();
    }

    bool ResourceManager::is_texture_loaded(ResourceID id) const
    {
        return impl_->textures.find(id) != impl_->textures.end();
    }

    bool ResourceManager::is_material_loaded(ResourceID id) const
    {
        return impl_->materials.find(id) != impl_->materials.end();
    }

    bool ResourceManager::is_shader_loaded(ResourceID id) const
    {
        return impl_->shaders.find(id) != impl_->shaders.end();
    }

    void ResourceManager::register_mesh_loader(const std::string& extension, MeshLoader loader)
    {
        // Placeholder: Mesh loader registry
    }

    void ResourceManager::register_texture_loader(const std::string& extension, TextureLoader loader)
    {
        // Placeholder: Texture loader registry
    }

    void ResourceManager::update_hot_reloads()
    {
        // Placeholder: Check for modified shaders and reload them
    }

    void ResourceManager::enable_hot_reloading(bool enable)
    {
        // Placeholder: Enable/disable hot reloading
    }

    void ResourceManager::load_async(std::function<void()> load_func)
    {
        std::lock_guard<std::mutex> lock(impl_->queue_mutex);
        impl_->load_queue.push(std::move(load_func));
    }

    void ResourceManager::wait_for_all_loads()
    {
        // Placeholder: Wait for all async loading to complete
    }
} // namespace vulkan_engine::rendering
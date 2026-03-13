#include "rendering/scene/Scene.hpp"

namespace vulkan_engine::rendering
{
    struct Scene::Impl
    {
        std::string name;

        // Entity storage
        std::vector<std::unique_ptr<Entity>>     entities;
        std::unordered_map<std::string, Entity*> entity_map;

        // Spatial indexing
        std::unique_ptr<SpatialIndex> spatial_index;

        // Scene state
        bool is_loaded = false;
    };

    Scene::Scene(const std::string& name)
        : impl_(std::make_unique<Impl>())
    {
        impl_->name = name;
    }

    Scene::~Scene() = default;

    Entity* Scene::create_entity(const std::string& name)
    {
        auto entity  = std::make_unique<Entity>();
        entity->id   = static_cast<uint32_t>(impl_->entities.size());
        entity->name = name;

        Entity* ptr = entity.get();
        impl_->entities.push_back(std::move(entity));
        impl_->entity_map[name] = ptr;

        return ptr;
    }

    void Scene::destroy_entity(Entity* entity)
    {
        if (!entity) return;

        // Find and remove entity
        auto it = impl_->entity_map.find(entity->name);
        if (it != impl_->entity_map.end())
        {
            impl_->entity_map.erase(it);
        }

        // Remove from entities vector
        auto entity_it = std::find_if(impl_->entities.begin(),
                                      impl_->entities.end(),
                                      [entity](const auto& e) { return e.get() == entity; });

        if (entity_it != impl_->entities.end())
        {
            impl_->entities.erase(entity_it);
        }
    }

    Entity* Scene::get_entity(const std::string& name)
    {
        auto it = impl_->entity_map.find(name);
        return (it != impl_->entity_map.end()) ? it->second : nullptr;
    }

    const std::vector<std::unique_ptr<Entity>>& Scene::get_entities() const
    {
        return impl_->entities;
    }

    void Scene::clear()
    {
        impl_->entities.clear();
        impl_->entity_map.clear();
    }

    bool Scene::load()
    {
        impl_->is_loaded = true;
        return true;
    }

    void Scene::unload()
    {
        clear();
        impl_->is_loaded = false;
    }

    bool Scene::is_loaded() const
    {
        return impl_->is_loaded;
    }

    void Scene::update(float delta_time)
    {
        // Update all entities
        for (auto& entity : impl_->entities)
        {
            // Update entity components
        }
    }

    void Scene::render(RenderContext& context)
    {
        // Render all visible entities
        for (auto& entity : impl_->entities)
        {
            // Render entity
        }
    }

    void Scene::set_name(const std::string& name)
    {
        impl_->name = name;
    }

    std::string Scene::get_name() const
    {
        return impl_->name;
    }

    void Scene::set_spatial_index(std::unique_ptr<SpatialIndex> index)
    {
        impl_->spatial_index = std::move(index);
    }

    SpatialIndex* Scene::get_spatial_index() const
    {
        return impl_->spatial_index.get();
    }

    void Scene::frustum_cull(const Frustum& frustum, std::vector<Entity*>& out_visible)
    {
        if (impl_->spatial_index)
        {
            impl_->spatial_index->frustum_cull(frustum, out_visible);
        }
        else
        {
            // No spatial indexing, return all entities
            for (auto& entity : impl_->entities)
            {
                out_visible.push_back(entity.get());
            }
        }
    }

    RaycastResult Scene::raycast(const Ray& ray, float max_distance)
    {
        RaycastResult result;
        result.hit      = false;
        result.distance = max_distance;

        if (impl_->spatial_index)
        {
            return impl_->spatial_index->raycast(ray, max_distance);
        }

        // Simple brute force raycast
        for (auto& entity : impl_->entities)
        {
            // Check intersection with entity bounds
            // This is a placeholder - actual implementation would check mesh intersection
        }

        return result;
    }
} // namespace vulkan_engine::rendering
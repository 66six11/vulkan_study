#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace vulkan_engine::rendering
{
    struct Entity
    {
        uint32_t    id = 0;
        std::string name;
        // Transform, components, etc.
    };

    struct RenderContext
    {
        // View matrix, projection matrix, camera, etc.
    };

    struct Frustum
    {
        // Frustum planes for culling
    };

    struct Ray
    {
        // Origin and direction
    };

    struct RaycastResult
    {
        bool    hit      = false;
        float   distance = 0.0f;
        Entity* entity   = nullptr;
    };

    class SpatialIndex
    {
        public:
            virtual ~SpatialIndex() = default;

            virtual void          insert(Entity* entity) = 0;
            virtual void          remove(Entity* entity) = 0;
            virtual void          update(Entity* entity) = 0;
            virtual void          clear() = 0;
            virtual void          frustum_cull(const Frustum& frustum, std::vector<Entity*>& out_visible) = 0;
            virtual RaycastResult raycast(const Ray& ray, float max_distance) = 0;
    };

    class Scene
    {
        public:
            explicit Scene(const std::string& name = "Untitled");
            ~Scene();

            // Entity management
            Entity*                                     create_entity(const std::string& name);
            void                                        destroy_entity(Entity* entity);
            Entity*                                     get_entity(const std::string& name);
            const std::vector<std::unique_ptr<Entity>>& get_entities() const;
            void                                        clear();

            // Scene lifecycle
            bool load();
            void unload();
            bool is_loaded() const;

            // Update and render
            void update(float delta_time);
            void render(RenderContext& context);

            // Properties
            void        set_name(const std::string& name);
            std::string get_name() const;

            // Spatial indexing
            void          set_spatial_index(std::unique_ptr<SpatialIndex> index);
            SpatialIndex* get_spatial_index() const;

            // Queries
            void          frustum_cull(const Frustum& frustum, std::vector<Entity*>& out_visible);
            RaycastResult raycast(const Ray& ray, float max_distance);

        private:
            struct Impl;
            std::unique_ptr<Impl> impl_;
    };
} // namespace vulkan_engine::rendering
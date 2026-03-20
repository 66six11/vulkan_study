#pragma once

#include "engine/rhi/vulkan/resources/Buffer.hpp"
#include "engine/rhi/vulkan/command/CommandBuffer.hpp"
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>

namespace vulkan_engine::rendering
{
    // Simple vertex with position, normal, uv, and color
    struct MeshVertex
    {
        glm::vec3 position{0.0f};
        glm::vec3 normal{0.0f, 1.0f, 0.0f};
        glm::vec2 uv{0.0f};
        glm::vec3 color{1.0f};
    };

    // CPU-side mesh data
    struct MeshData
    {
        std::vector<MeshVertex> vertices;
        std::vector<uint32_t>   indices;
        std::string             name;

        void clear()
        {
            vertices.clear();
            indices.clear();
            name.clear();
        }

        bool is_empty() const { return vertices.empty() || indices.empty(); }
    };

    // GPU-ready mesh with buffers
    class Mesh
    {
        public:
            Mesh()  = default;
            ~Mesh() = default;

            // Non-copyable
            Mesh(const Mesh&)            = delete;
            Mesh& operator=(const Mesh&) = delete;

            // Movable
            Mesh(Mesh&&)            = default;
            Mesh& operator=(Mesh&&) = default;

            // Upload mesh data to GPU
            void upload(std::shared_ptr<vulkan::DeviceManager> device, const MeshData& data);

            // Bind for rendering
            void bind(vulkan::RenderCommandBuffer& cmd);

            // Draw
            void draw(vulkan::RenderCommandBuffer& cmd);

            // Getters
            uint32_t vertex_count() const { return vertex_count_; }
            uint32_t index_count() const { return index_count_; }
            bool     is_uploaded() const { return vertex_buffer_ != nullptr; }

            const std::string& name() const { return name_; }
            void               set_name(const std::string& name) { name_ = name; }

            // Buffer access for integration with existing render passes
            vulkan::Buffer* vertex_buffer() const { return vertex_buffer_.get(); }
            vulkan::Buffer* index_buffer() const { return index_buffer_.get(); }

        private:
            std::shared_ptr<vulkan::DeviceManager> device_;
            std::unique_ptr<vulkan::Buffer>        vertex_buffer_;
            std::unique_ptr<vulkan::Buffer>        index_buffer_;
            uint32_t                               vertex_count_ = 0;
            uint32_t                               index_count_  = 0;
            std::string                            name_;
    };
} // namespace vulkan_engine::rendering
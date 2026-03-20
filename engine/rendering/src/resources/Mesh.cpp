#include "rendering/resources/Mesh.hpp"
#include "core/utils/Logger.hpp"
#include <cstring>

namespace vulkan_engine::rendering
{
    void Mesh::upload(std::shared_ptr<vulkan::DeviceManager> device, const MeshData& data)
    {
        device_ = device;
        name_   = data.name;

        if (data.is_empty())
        {
            logger::warn("Mesh::upload called with empty mesh data");
            return;
        }

        // Create vertex buffer
        VkDeviceSize vertex_buffer_size = sizeof(MeshVertex) * data.vertices.size();
        vertex_buffer_                  = std::make_unique<vulkan::Buffer>(
                                                          device,
                                                          vertex_buffer_size,
                                                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void* vertex_data = vertex_buffer_->map();
        memcpy(vertex_data, data.vertices.data(), static_cast<size_t>(vertex_buffer_size));
        vertex_buffer_->unmap();

        vertex_count_ = static_cast<uint32_t>(data.vertices.size());

        // Create index buffer
        VkDeviceSize index_buffer_size = sizeof(uint32_t) * data.indices.size();
        index_buffer_                  = std::make_unique<vulkan::Buffer>(
                                                         device,
                                                         index_buffer_size,
                                                         VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        void* index_data = index_buffer_->map();
        memcpy(index_data, data.indices.data(), static_cast<size_t>(index_buffer_size));
        index_buffer_->unmap();

        index_count_ = static_cast<uint32_t>(data.indices.size());

        logger::info("Mesh '" + name_ + "' uploaded to GPU: " +
                     std::to_string(vertex_count_) + " vertices, " +
                     std::to_string(index_count_) + " indices");
    }

    void Mesh::bind(vulkan::RenderCommandBuffer& cmd)
    {
        if (!is_uploaded())
        {
            return;
        }

        cmd.bind_vertex_buffer(vertex_buffer_->handle(), 0);
        cmd.bind_index_buffer(index_buffer_->handle(), VK_INDEX_TYPE_UINT32);
    }

    void Mesh::draw(vulkan::RenderCommandBuffer& cmd)
    {
        if (!is_uploaded() || index_count_ == 0)
        {
            return;
        }

        cmd.draw_indexed(index_count_, 1, 0, 0, 0);
    }
} // namespace vulkan_engine::rendering
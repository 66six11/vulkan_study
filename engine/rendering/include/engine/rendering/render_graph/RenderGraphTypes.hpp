#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>
#include <string>
#include <vector>

namespace vulkan_engine::rendering
{
    // Resource handle - type-safe resource identifier
    template <typename T> class ResourceHandle
    {
        public:
            constexpr ResourceHandle() noexcept
                : id_{0}, generation_{0}
            {
            }

            constexpr ResourceHandle(uint32_t id, uint32_t generation) noexcept
                : id_{id}, generation_{generation}
            {
            }

            template <typename U> constexpr ResourceHandle(ResourceHandle<U> other) noexcept
                : id_{other.id()}, generation_{other.generation()}
            {
            }

            constexpr bool     valid() const noexcept { return id_ != 0; }
            constexpr uint32_t id() const noexcept { return id_; }
            constexpr uint32_t generation() const noexcept { return generation_; }

            constexpr bool operator==(const ResourceHandle& other) const noexcept
            {
                return id_ == other.id_ && generation_ == other.generation_;
            }

            constexpr bool operator!=(const ResourceHandle& other) const noexcept
            {
                return !(*this == other);
            }

        private:
            uint32_t id_;
            uint32_t generation_;
    };

    // Resource types
    struct BufferResource
    {
    };

    struct ImageResource
    {
    };

    struct TextureResource
    {
    };

    using BufferHandle  = ResourceHandle<BufferResource>;
    using ImageHandle   = ResourceHandle<ImageResource>;
    using TextureHandle = ResourceHandle<TextureResource>;

    // Resource description
    struct ResourceDesc
    {
        std::string name;

        enum class Type
        {
            Buffer,
            Image,
            Texture
        } type;

        // Common properties
        uint32_t width        = 0;
        uint32_t height       = 0;
        uint32_t depth        = 1;
        uint32_t array_layers = 1;
        uint32_t mip_levels   = 1;

        // Usage flags
        bool is_transient = false;
        bool is_external  = false;

        // Specific to buffer resources
        uint64_t size = 0;

        // Specific to image resources
        enum class Format
        {
            R8G8B8A8_UNORM,
            R16G16B16A16_SFLOAT,
            D32_SFLOAT
        } format = Format::R8G8B8A8_UNORM;
    };

    // Barrier batch for efficient submission
    struct BarrierBatch
    {
        std::vector<VkImageMemoryBarrier>  image_barriers;
        std::vector<VkBufferMemoryBarrier> buffer_barriers;
        VkPipelineStageFlags               src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags               dst_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        bool empty() const { return image_barriers.empty() && buffer_barriers.empty(); }

        void clear()
        {
            image_barriers.clear();
            buffer_barriers.clear();
            src_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dst_stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }
    };
} // namespace vulkan_engine::rendering
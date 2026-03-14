#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <concepts>
#include <functional>

namespace vulkan_engine::vulkan
{
    class RenderCommandBuffer;
}

namespace vulkan_engine::rendering
{
    // Forward declarations
    class CommandBuffer;
    struct RenderContext;

    // Resource handle - type-safe resource identifier
    template <typename T> class ResourceHandle
    {
        public:
            constexpr ResourceHandle() noexcept : id_{0}, generation_{0}
            {
            }

            constexpr ResourceHandle(uint32_t id, uint32_t generation) noexcept
                : id_{id}, generation_{generation}
            {
            }

            // Allow conversion from other resource handle types (for internal storage)
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

    // Render pass concept
    template <typename T>
    concept RenderPass = requires(T t, CommandBuffer& cmd)
    {
        { t.execute(cmd) } -> std::same_as<void>;
        { t.name() } -> std::convertible_to<std::string_view>;
    };

    // Render graph node
    class RenderGraphNode
    {
        public:
            virtual ~RenderGraphNode() = default;

            virtual void             execute(CommandBuffer& cmd) = 0;
            virtual std::string_view name() const = 0;

            // Resource dependencies
            virtual std::vector<BufferHandle> get_buffer_inputs() const = 0;
            virtual std::vector<ImageHandle>  get_image_inputs() const = 0;
            virtual std::vector<BufferHandle> get_buffer_outputs() const = 0;
            virtual std::vector<ImageHandle>  get_image_outputs() const = 0;
    };

    // Forward declaration
    class RenderGraph;

    // Render graph builder
    class RenderGraphBuilder
    {
        public:
            // Resource creation
            BufferHandle  create_buffer(const ResourceDesc& desc);
            ImageHandle   create_image(const ResourceDesc& desc);
            TextureHandle create_texture(const ResourceDesc& desc);

            // Pass creation
            template <RenderPass Pass> void add_pass(Pass&& pass)
            {
                auto node = std::make_unique<std::decay_t<Pass>>(std::forward<Pass>(pass));
                nodes_.push_back(std::move(node));
            }

            // Add a render graph node directly
            void add_node(std::unique_ptr<RenderGraphNode> node)
            {
                nodes_.push_back(std::move(node));
            }

            // Resource access
            void read(BufferHandle buffer);
            void write(BufferHandle buffer);
            void read(ImageHandle image);
            void write(ImageHandle image);

            // Allow RenderGraph to access nodes
            const std::vector<std::unique_ptr<RenderGraphNode>>& nodes() const { return nodes_; }

        private:
            std::vector<std::unique_ptr<RenderGraphNode>>         nodes_;
            std::unordered_map<std::string, ResourceHandle<void>> resources_;

            friend class RenderGraph;
    };

    // Forward declarations
    struct RenderContext;

    // Main render graph class
    class RenderGraph
    {
        public:
            RenderGraph()  = default;
            ~RenderGraph() = default;

            // Compile the render graph
            void compile();

            // Execute the render graph (basic version)
            void execute();

            // Execute with Vulkan command buffer and context
            void execute(vulkan::RenderCommandBuffer& cmd, const RenderContext& ctx);

            // Get the builder for constructing the graph
            RenderGraphBuilder& builder() { return builder_; }

            // Resource access
            template <typename T> ResourceHandle<T> get_resource(const std::string& name) const;

            // Check if compiled
            bool is_compiled() const { return compiled_; }

            // Reset compilation state
            void reset()
            {
                compiled_ = false;
                execution_order_.clear();
            }

        private:
            RenderGraphBuilder            builder_;
            bool                          compiled_ = false;
            std::vector<RenderGraphNode*> execution_order_;
    };

    // Example render pass implementation
    template <typename ExecuteFunc> class LambdaRenderPass : public RenderGraphNode
    {
        public:
            LambdaRenderPass(std::string name, ExecuteFunc&& func)
                : name_{std::move(name)}, func_{std::forward<ExecuteFunc>(func)}
            {
            }

            void execute(CommandBuffer& cmd) override
            {
                func_(cmd);
            }

            std::string_view name() const override
            {
                return name_;
            }

            std::vector<BufferHandle> get_buffer_inputs() const override { return {}; }
            std::vector<ImageHandle>  get_image_inputs() const override { return {}; }
            std::vector<BufferHandle> get_buffer_outputs() const override { return {}; }
            std::vector<ImageHandle>  get_image_outputs() const override { return {}; }

        private:
            std::string name_;
            ExecuteFunc func_;
    };
} // namespace vulkan_engine::rendering
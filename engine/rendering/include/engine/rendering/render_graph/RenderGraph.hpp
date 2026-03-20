#pragma once

#include "rendering/render_graph/RenderGraphTypes.hpp"
#include "vulkan/device/Device.hpp"

#include <vulkan/vulkan.h>
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
    class RenderGraphResourcePool;
    class BarrierManager;
    class RenderGraphNode;

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
            RenderGraph();
            ~RenderGraph();

            // Non-copyable
            RenderGraph(const RenderGraph&)            = delete;
            RenderGraph& operator=(const RenderGraph&) = delete;

            // Movable
            RenderGraph(RenderGraph&& other) noexcept;
            RenderGraph& operator=(RenderGraph&& other) noexcept;

            // Initialize with device
            void initialize(std::shared_ptr<vulkan::DeviceManager> device);

            // Compile the render graph - analyzes dependencies and generates barriers
            void compile();

            // Execute the render graph (basic version)
            void execute();

            // Execute with Vulkan command buffer and context
            void execute(vulkan::RenderCommandBuffer& cmd, const RenderContext& ctx);

            // Get the builder for constructing the graph
            RenderGraphBuilder& builder() { return builder_; }

            // Resource access
            template <typename T> ResourceHandle<T> get_resource(const std::string& name) const;

            // Create a transient image resource
            ImageHandle create_image(const ResourceDesc& desc);

            // Create a transient buffer resource
            BufferHandle create_buffer(const ResourceDesc& desc);

            // Import external image (e.g., swap chain)
            ImageHandle import_image(
                VkImage            image,
                VkImageView        view,
                VkFormat           format,
                uint32_t           width,
                uint32_t           height,
                const std::string& name);

            // Check if compiled
            bool is_compiled() const { return compiled_; }

            // Reset compilation state and release resources
            void reset();

            // Get resource pool
            RenderGraphResourcePool* resource_pool() const { return resource_pool_.get(); }

        private:
            RenderGraphBuilder            builder_;
            bool                          compiled_ = false;
            std::vector<RenderGraphNode*> execution_order_;

            // Resource management
            std::shared_ptr<vulkan::DeviceManager>   device_;
            std::unique_ptr<RenderGraphResourcePool> resource_pool_;
            std::unique_ptr<BarrierManager>          barrier_manager_;

            // Resource handle generation
            uint32_t next_resource_id_ = 1;

            // Per-pass barrier batches
            std::vector<BarrierBatch> pass_barriers_;

            // Analyze dependencies and build execution order
            void build_execution_order();

            // Generate barriers for all passes
            void generate_barriers();
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
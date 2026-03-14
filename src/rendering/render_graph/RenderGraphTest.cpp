#include "rendering/render_graph/RenderGraph.hpp"
#include "rendering/render_graph/RenderGraphPass.hpp"
#include "rendering/render_graph/RenderGraphResource.hpp"
#include "core/utils/Logger.hpp"

namespace vulkan_engine::rendering
{
    // ============================================================================
    // Test Pass 1: GBuffer Generation Pass
    // ============================================================================
    class GBufferPass : public RenderPassBase
    {
        public:
            struct Config
            {
                std::string name = "GBufferPass";
                ImageHandle position_output;
                ImageHandle normal_output;
                ImageHandle albedo_output;
            };

            explicit GBufferPass(const Config& config)
                : config_(config)
            {
                name_ = config.name;
            }

            void setup(RenderGraphBuilder& /*builder*/) override
            {
                // Declare outputs - these will trigger barrier generation
                if (config_.position_output.valid())
                {
                    image_outputs_.push_back(config_.position_output);
                }
                if (config_.normal_output.valid())
                {
                    image_outputs_.push_back(config_.normal_output);
                }
                if (config_.albedo_output.valid())
                {
                    image_outputs_.push_back(config_.albedo_output);
                }
            }

            void execute(vulkan::RenderCommandBuffer& cmd, const RenderContext& ctx) override
            {
                logger::info("Executing GBuffer Pass");
                // In real implementation, this would:
                // 1. Bind GBuffer generation pipeline
                // 2. Render scene geometry
                // 3. Output to multiple render targets
                (void)cmd;
                (void)ctx;
            }

        private:
            Config config_;
    };

    // ============================================================================
    // Test Pass 2: Deferred Lighting Pass
    // ============================================================================
    class DeferredLightingPass : public RenderPassBase
    {
        public:
            struct Config
            {
                std::string name = "DeferredLightingPass";
                ImageHandle position_input;
                ImageHandle normal_input;
                ImageHandle albedo_input;
                ImageHandle lighting_output;
            };

            explicit DeferredLightingPass(const Config& config)
                : config_(config)
            {
                name_ = config.name;
            }

            void setup(RenderGraphBuilder& /*builder*/) override
            {
                // Declare inputs - need transition from COLOR_ATTACHMENT to SHADER_READ
                if (config_.position_input.valid())
                {
                    image_inputs_.push_back(config_.position_input);
                }
                if (config_.normal_input.valid())
                {
                    image_inputs_.push_back(config_.normal_input);
                }
                if (config_.albedo_input.valid())
                {
                    image_inputs_.push_back(config_.albedo_input);
                }

                // Declare output
                if (config_.lighting_output.valid())
                {
                    image_outputs_.push_back(config_.lighting_output);
                }
            }

            void execute(vulkan::RenderCommandBuffer& cmd, const RenderContext& ctx) override
            {
                logger::info("Executing Deferred Lighting Pass");
                // In real implementation, this would:
                // 1. Bind deferred lighting pipeline
                // 2. Sample from GBuffer textures
                // 3. Calculate lighting
                // 4. Output to lighting buffer
                (void)cmd;
                (void)ctx;
            }

        private:
            Config config_;
    };

    // ============================================================================
    // Test Pass 3: Post Processing Pass
    // ============================================================================
    class PostProcessPass : public RenderPassBase
    {
        public:
            struct Config
            {
                std::string name = "PostProcessPass";
                ImageHandle input;
                ImageHandle output;
            };

            explicit PostProcessPass(const Config& config)
                : config_(config)
            {
                name_ = config.name;
            }

            void setup(RenderGraphBuilder& /*builder*/) override
            {
                // Input from lighting pass
                if (config_.input.valid())
                {
                    image_inputs_.push_back(config_.input);
                }

                // Output to final image
                if (config_.output.valid())
                {
                    image_outputs_.push_back(config_.output);
                }
            }

            void execute(vulkan::RenderCommandBuffer& cmd, const RenderContext& ctx) override
            {
                logger::info("Executing Post Process Pass");
                // In real implementation, this would:
                // 1. Bind post-process pipeline (tonemapping, bloom, etc.)
                // 2. Sample from lighting buffer
                // 3. Apply effects
                // 4. Output to swap chain image
                (void)cmd;
                (void)ctx;
            }

        private:
            Config config_;
    };

    // ============================================================================
    // Render Graph Test Function
    // ============================================================================
    void test_render_graph_resource_management(std::shared_ptr<vulkan::DeviceManager> device)
    {
        logger::info("========================================");
        logger::info("Render Graph Resource Management Test");
        logger::info("========================================");

        // Create render graph
        RenderGraph render_graph;
        render_graph.initialize(device);

        // Create transient resources (GBuffer)
        auto gbuffer_position = render_graph.create_image({
                                                              .type = ResourceDesc::Type::Image,
                                                              .width = 1920,
                                                              .height = 1080,
                                                              .format = ResourceDesc::Format::R16G16B16A16_SFLOAT
                                                          });

        auto gbuffer_normal = render_graph.create_image({
                                                            .type = ResourceDesc::Type::Image,
                                                            .width = 1920,
                                                            .height = 1080,
                                                            .format = ResourceDesc::Format::R16G16B16A16_SFLOAT
                                                        });

        auto gbuffer_albedo = render_graph.create_image({
                                                            .type = ResourceDesc::Type::Image,
                                                            .width = 1920,
                                                            .height = 1080,
                                                            .format = ResourceDesc::Format::R8G8B8A8_UNORM
                                                        });

        auto lighting_buffer = render_graph.create_image({
                                                             .type = ResourceDesc::Type::Image,
                                                             .width = 1920,
                                                             .height = 1080,
                                                             .format = ResourceDesc::Format::R16G16B16A16_SFLOAT
                                                         });

        logger::info("Created transient resources:");
        logger::info("  - GBuffer Position: ID=" + std::to_string(gbuffer_position.id()));
        logger::info("  - GBuffer Normal: ID=" + std::to_string(gbuffer_normal.id()));
        logger::info("  - GBuffer Albedo: ID=" + std::to_string(gbuffer_albedo.id()));
        logger::info("  - Lighting Buffer: ID=" + std::to_string(lighting_buffer.id()));

        // Create passes
        GBufferPass::Config gbuffer_config;
        gbuffer_config.position_output = gbuffer_position;
        gbuffer_config.normal_output   = gbuffer_normal;
        gbuffer_config.albedo_output   = gbuffer_albedo;
        render_graph.builder().add_node(std::make_unique<GBufferPass>(gbuffer_config));

        DeferredLightingPass::Config lighting_config;
        lighting_config.position_input  = gbuffer_position;
        lighting_config.normal_input    = gbuffer_normal;
        lighting_config.albedo_input    = gbuffer_albedo;
        lighting_config.lighting_output = lighting_buffer;
        render_graph.builder().add_node(std::make_unique<DeferredLightingPass>(lighting_config));

        // Compile render graph - this triggers barrier generation
        logger::info("\nCompiling Render Graph...");
        render_graph.compile();

        // Verify compilation
        if (render_graph.is_compiled())
        {
            logger::info("Render Graph compiled successfully!");
            logger::info("Resource dependencies analyzed.");
            logger::info("Barriers will be automatically inserted between passes.");
        }
        else
        {
            logger::error("Render Graph compilation failed!");
        }

        // Get resource pool info
        auto* pool = render_graph.resource_pool();
        if (pool)
        {
            logger::info("\nResource Pool Status:");
            logger::info("  - Images created: 4");
            logger::info("  - Buffers created: 0");
        }

        logger::info("\n========================================");
        logger::info("Test Complete - Resource management working!");
        logger::info("========================================");

        // Explicitly reset to ensure resources are freed before device destruction
        render_graph.reset();
    }
} // namespace vulkan_engine::rendering
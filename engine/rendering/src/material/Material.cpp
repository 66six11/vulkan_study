#include "engine/rendering/material/Material.hpp"
#include "engine/core/utils/Logger.hpp"
#include "engine/rhi/vulkan/utils/VulkanError.hpp"

#include <glm/glm.hpp>

namespace vulkan_engine::rendering
{
    Material::Material(std::shared_ptr<vulkan::DeviceManager> device, const Config& config)
        : device_(std::move(device)), config_(config)
    {
        // Initialize uniform buffer with default values
        uniform_buffer_ = std::make_unique<vulkan::UniformBuffer<UniformBufferData>>(device_, 1);

        // Upload initial default values
        UniformBufferData initial_data{};
        uniform_buffer_->update(0, initial_data);

        // Create descriptor set layout and pipeline layout
        create_descriptor_set_layout();
        create_pipeline_layout();
        create_descriptor_set();
        create_default_sampler();
        create_default_white_texture();
    }

    Material::~Material()
    {
        cleanup();
    }

    Material::Material(Material&& other) noexcept
        : device_(std::move(other.device_))
        , config_(std::move(other.config_))
        , pipeline_(std::move(other.pipeline_))
        , pipeline_layout_(other.pipeline_layout_)
        , descriptor_set_layout_(other.descriptor_set_layout_)
        , uniform_buffer_(std::move(other.uniform_buffer_))
        , descriptor_pool_(other.descriptor_pool_)
        , descriptor_set_(other.descriptor_set_)
        , textures_(std::move(other.textures_))
        , default_sampler_(other.default_sampler_)
        , default_white_texture_(std::move(other.default_white_texture_))
        , default_white_texture_view_(other.default_white_texture_view_)
    {
        other.pipeline_layout_            = VK_NULL_HANDLE;
        other.descriptor_set_layout_      = VK_NULL_HANDLE;
        other.descriptor_pool_            = VK_NULL_HANDLE;
        other.descriptor_set_             = VK_NULL_HANDLE;
        other.default_sampler_            = VK_NULL_HANDLE;
        other.default_white_texture_view_ = VK_NULL_HANDLE;
    }

    Material& Material::operator=(Material&& other) noexcept
    {
        if (this != &other)
        {
            cleanup();

            device_                     = std::move(other.device_);
            config_                     = std::move(other.config_);
            pipeline_                   = std::move(other.pipeline_);
            pipeline_layout_            = other.pipeline_layout_;
            descriptor_set_layout_      = other.descriptor_set_layout_;
            uniform_buffer_             = std::move(other.uniform_buffer_);
            descriptor_pool_            = other.descriptor_pool_;
            descriptor_set_             = other.descriptor_set_;
            textures_                   = std::move(other.textures_);
            default_sampler_            = other.default_sampler_;
            default_white_texture_      = std::move(other.default_white_texture_);
            default_white_texture_view_ = other.default_white_texture_view_;

            other.pipeline_layout_            = VK_NULL_HANDLE;
            other.descriptor_set_layout_      = VK_NULL_HANDLE;
            other.descriptor_pool_            = VK_NULL_HANDLE;
            other.descriptor_set_             = VK_NULL_HANDLE;
            other.default_sampler_            = VK_NULL_HANDLE;
            other.default_white_texture_view_ = VK_NULL_HANDLE;
        }
        return *this;
    }

    void Material::cleanup()
    {
        if (!device_ || device_->device() == VK_NULL_HANDLE)
        {
            return;
        }

        VkDevice device = device_->device();

        // Destroy descriptor pool
        if (descriptor_pool_ != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device, descriptor_pool_, nullptr);
            descriptor_pool_ = VK_NULL_HANDLE;
        }

        // Destroy pipeline
        pipeline_.reset();

        // Destroy pipeline layout
        if (pipeline_layout_ != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(device, pipeline_layout_, nullptr);
            pipeline_layout_ = VK_NULL_HANDLE;
        }

        // Destroy descriptor set layout
        if (descriptor_set_layout_ != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(device, descriptor_set_layout_, nullptr);
            descriptor_set_layout_ = VK_NULL_HANDLE;
        }

        // Destroy default sampler
        if (default_sampler_ != VK_NULL_HANDLE)
        {
            vkDestroySampler(device, default_sampler_, nullptr);
            default_sampler_ = VK_NULL_HANDLE;
        }

        // Destroy uniform buffer
        uniform_buffer_.reset();

        // Clear textures (shared_ptr will handle cleanup)
        textures_.clear();

        // Clear default white texture
        default_white_texture_.reset();
        default_white_texture_view_ = VK_NULL_HANDLE;
    }

    void Material::build(VkFormat color_format, VkFormat depth_format)
    {
        if (color_format == VK_FORMAT_UNDEFINED)
        {
            logger::error("Material " + config_.name + " cannot build: no color format provided");
            return;
        }

        // Check if already built
        if (pipeline_ != nullptr)
        {
            logger::warn("Material " + config_.name + " already built, skipping...");
            return;
        }

        build_internal(color_format, depth_format);
    }

    void Material::build_internal(VkFormat color_format, VkFormat depth_format)
    {
        // Update descriptor set
        update_descriptor_set();

        // Create pipeline config for dynamic rendering
        vulkan::GraphicsPipelineConfig pipeline_config{};
        pipeline_config.render_pass          = VK_NULL_HANDLE; // Dynamic rendering
        pipeline_config.color_format         = color_format;
        pipeline_config.depth_format         = depth_format;
        pipeline_config.vertex_shader_path   = config_.vertex_shader_path;
        pipeline_config.fragment_shader_path = config_.fragment_shader_path;
        pipeline_config.layout               = pipeline_layout_;

        // Vertex input matching MeshVertex: position(3) + normal(3) + uv(2) + color(3) = 11 floats = 44 bytes
        pipeline_config.vertex_bindings = {
            {0, sizeof(float) * 11, VK_VERTEX_INPUT_RATE_VERTEX}
        };

        pipeline_config.vertex_attributes = {
            {0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0},
            // position at location 0, offset 0
            {1, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 3},
            // normal at location 1, offset 12
            {2, 0, VK_FORMAT_R32G32_SFLOAT, sizeof(float) * 6},
            // uv at location 2, offset 24
            {3, 0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(float) * 8} // color at location 3, offset 32
        };

        pipeline_config.primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        pipeline_config.polygon_mode       = VK_POLYGON_MODE_FILL;
        pipeline_config.cull_mode          = config_.cull_mode;
        pipeline_config.front_face         = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        pipeline_config.depth_test_enable  = config_.depth_test;
        pipeline_config.depth_write_enable = config_.depth_write;
        pipeline_config.depth_compare_op   = config_.depth_compare_op;
        pipeline_config.blend_enable       = config_.blend_enable;

        try
        {
            pipeline_ = std::make_unique<vulkan::GraphicsPipeline>(device_, pipeline_config);
            logger::info("Material " + config_.name + " built successfully");
        }
        catch (const std::exception& e)
        {
            logger::error("Failed to build material " + config_.name + ": " + e.what());
            throw;
        }
    }

    void Material::bind(vulkan::RenderCommandBuffer& cmd)
    {
        if (!pipeline_)
        {
            logger::error("Material " + config_.name + " not built, cannot bind");
            return;
        }

        // Bind pipeline
        cmd.bind_graphics_pipeline(*pipeline_);

        // Bind descriptor set
        if (descriptor_set_ != VK_NULL_HANDLE)
        {
            cmd.bind_descriptor_sets(pipeline_layout_, 0, {descriptor_set_});
        }
    }

    void Material::set_float(const std::string& name, float value)
    {
        std::lock_guard<std::mutex> lock(uniform_mutex_);
        UniformBufferData           data = uniform_buffer_->get_data(0);
        if (name == "roughness")
            data.roughness = value;
        else if (name == "metallic")
            data.metallic = value;
        else if (name == "emissive")
            data.emissive = value;
        uniform_buffer_->update(0, data);
    }

    void Material::set_vec3(const std::string& name, const glm::vec3& value)
    {
        if (name == "color")
        {
            std::lock_guard<std::mutex> lock(uniform_mutex_);
            UniformBufferData           data = uniform_buffer_->get_data(0);
            data.color                       = glm::vec4(value, 1.0f);
            uniform_buffer_->update(0, data);
        }
    }

    void Material::set_vec4(const std::string& name, const glm::vec4& value)
    {
        if (name == "color")
        {
            std::lock_guard<std::mutex> lock(uniform_mutex_);
            UniformBufferData           data = uniform_buffer_->get_data(0);
            data.color                       = value;
            uniform_buffer_->update(0, data);
        }
    }

    void Material::set_vec2(const std::string& name, const glm::vec2& value)
    {
        std::lock_guard<std::mutex> lock(uniform_mutex_);
        UniformBufferData           data = uniform_buffer_->get_data(0);
        if (name == "uv_scale")
        {
            data.uv_scale = value;
        }
        else if (name == "uv_offset")
        {
            // Store in uv_scale's neighboring field or add separate field if needed
            // For now, we can use the padding area or extend the struct
            data.uv_scale = value; // Placeholder: use uv_scale for now
        }
        uniform_buffer_->update(0, data);
    }

    void Material::set_int(const std::string& name, int value)
    {
        std::lock_guard<std::mutex> lock(uniform_mutex_);
        UniformBufferData           data = uniform_buffer_->get_data(0);
        if (name == "texture_id")
        {
            data.texture_id = value;
        }
        uniform_buffer_->update(0, data);
    }

    void Material::set_bool(const std::string& name, bool value)
    {
        std::lock_guard<std::mutex> lock(uniform_mutex_);
        UniformBufferData           data = uniform_buffer_->get_data(0);
        if (name == "use_normal_map")
        {
            data.use_normal_map = value ? 1 : 0;
        }
        uniform_buffer_->update(0, data);
    }

    void Material::set_texture(const std::string& name, std::shared_ptr<vulkan::Image> texture, VkImageView view)
    {
        uint32_t binding = 0;
        if (name == "albedo") binding = 1;
        else if (name == "normal") binding = 2;
        else if (name == "roughness") binding = 3;
        else if (name == "metallic") binding = 4;

        textures_[name] = {std::move(texture), view, binding};

        // Update has_texture flag in uniform buffer
        {
            std::lock_guard<std::mutex> lock(uniform_mutex_);
            UniformBufferData           data = uniform_buffer_->get_data(0);
            data.has_texture                 = 1.0f;
            uniform_buffer_->update(0, data);
        }

        // Update descriptor set if already created
        if (descriptor_set_ != VK_NULL_HANDLE)
        {
            update_descriptor_set();
        }
    }

    void Material::create_descriptor_set_layout()
    {
        // Uniform buffer binding (binding 0)
        VkDescriptorSetLayoutBinding ubo_binding{};
        ubo_binding.binding            = 0;
        ubo_binding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubo_binding.descriptorCount    = 1;
        ubo_binding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        ubo_binding.pImmutableSamplers = nullptr;

        // Texture bindings (bindings 1-4)
        VkDescriptorSetLayoutBinding texture_bindings[4] = {};
        for (int i = 0; i < 4; ++i)
        {
            texture_bindings[i].binding            = i + 1;
            texture_bindings[i].descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            texture_bindings[i].descriptorCount    = 1;
            texture_bindings[i].stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
            texture_bindings[i].pImmutableSamplers = nullptr;
        }

        // Combine all bindings
        VkDescriptorSetLayoutBinding all_bindings[5] = {
            ubo_binding,
            texture_bindings[0],
            texture_bindings[1],
            texture_bindings[2],
            texture_bindings[3]
        };

        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = 5;
        layout_info.pBindings    = all_bindings;

        VkResult result = vkCreateDescriptorSetLayout(device_->device(), &layout_info, nullptr, &descriptor_set_layout_);
        if (result != VK_SUCCESS)
        {
            throw vulkan::VulkanError(result, "Failed to create material descriptor set layout", __FILE__, __LINE__);
        }
    }

    void Material::create_pipeline_layout()
    {
        // Push constant range for MVP matrix (vertex shader)
        VkPushConstantRange push_constant_range{};
        push_constant_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        push_constant_range.offset     = 0;
        push_constant_range.size       = sizeof(glm::mat4); // MVP matrix

        VkPipelineLayoutCreateInfo layout_info{};
        layout_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout_info.setLayoutCount         = 1;
        layout_info.pSetLayouts            = &descriptor_set_layout_;
        layout_info.pushConstantRangeCount = 1;
        layout_info.pPushConstantRanges    = &push_constant_range;

        VkResult result = vkCreatePipelineLayout(device_->device(), &layout_info, nullptr, &pipeline_layout_);
        if (result != VK_SUCCESS)
        {
            throw vulkan::VulkanError(result, "Failed to create material pipeline layout", __FILE__, __LINE__);
        }
    }

    void Material::create_descriptor_set()
    {
        // Create descriptor pool
        VkDescriptorPoolSize pool_sizes[2] = {};
        pool_sizes[0].type                 = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pool_sizes[0].descriptorCount      = 1;
        pool_sizes[1].type                 = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pool_sizes[1].descriptorCount      = 4;

        VkDescriptorPoolCreateInfo pool_info{};
        pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.poolSizeCount = 2;
        pool_info.pPoolSizes    = pool_sizes;
        pool_info.maxSets       = 1;

        VkResult result = vkCreateDescriptorPool(device_->device(), &pool_info, nullptr, &descriptor_pool_);
        if (result != VK_SUCCESS)
        {
            throw vulkan::VulkanError(result, "Failed to create material descriptor pool", __FILE__, __LINE__);
        }

        // Allocate descriptor set
        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool     = descriptor_pool_;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts        = &descriptor_set_layout_;

        result = vkAllocateDescriptorSets(device_->device(), &alloc_info, &descriptor_set_);
        if (result != VK_SUCCESS)
        {
            throw vulkan::VulkanError(result, "Failed to allocate material descriptor set", __FILE__, __LINE__);
        }
    }

    void Material::create_default_sampler()
    {
        VkSamplerCreateInfo sampler_info{};
        sampler_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_info.magFilter               = VK_FILTER_LINEAR;
        sampler_info.minFilter               = VK_FILTER_LINEAR;
        sampler_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        sampler_info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_info.anisotropyEnable        = VK_FALSE;
        sampler_info.maxAnisotropy           = 1.0f;
        sampler_info.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        sampler_info.unnormalizedCoordinates = VK_FALSE;
        sampler_info.compareEnable           = VK_FALSE;
        sampler_info.compareOp               = VK_COMPARE_OP_ALWAYS;
        sampler_info.mipLodBias              = 0.0f;
        sampler_info.minLod                  = 0.0f;
        sampler_info.maxLod                  = 0.0f;

        VkResult result = vkCreateSampler(device_->device(), &sampler_info, nullptr, &default_sampler_);
        if (result != VK_SUCCESS)
        {
            throw vulkan::VulkanError(result, "Failed to create default sampler", __FILE__, __LINE__);
        }
    }

    void Material::create_default_white_texture()
    {
        // Create a 1x1 white texture
        default_white_texture_ = std::make_shared<vulkan::Image>(
                                                                 device_,
                                                                 1,
                                                                 1,
                                                                 VK_FORMAT_R8G8B8A8_UNORM,
                                                                 VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                                                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                                 1,
                                                                 1);

        // Create staging buffer with white pixel data
        uint32_t white_pixel = 0xFFFFFFFF; // White (RGBA)

        VkBuffer       staging_buffer;
        VkDeviceMemory staging_memory;

        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size  = sizeof(white_pixel);
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

        vkCreateBuffer(device_->device(), &buffer_info, nullptr, &staging_buffer);

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(device_->device(), staging_buffer, &mem_requirements);

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize  = mem_requirements.size;
        alloc_info.memoryTypeIndex = device_->find_memory_type(
                                                               mem_requirements.memoryTypeBits,
                                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vkAllocateMemory(device_->device(), &alloc_info, nullptr, &staging_memory);
        vkBindBufferMemory(device_->device(), staging_buffer, staging_memory, 0);

        void* data;
        vkMapMemory(device_->device(), staging_memory, 0, sizeof(white_pixel), 0, &data);
        memcpy(data, &white_pixel, sizeof(white_pixel));
        vkUnmapMemory(device_->device(), staging_memory);

        // Create command buffer for layout transition and copy
        VkCommandPool   command_pool;
        VkCommandBuffer command_buffer;

        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.queueFamilyIndex = 0;
        pool_info.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        vkCreateCommandPool(device_->device(), &pool_info, nullptr, &command_pool);

        VkCommandBufferAllocateInfo cmd_alloc_info{};
        cmd_alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_alloc_info.commandPool        = command_pool;
        cmd_alloc_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_alloc_info.commandBufferCount = 1;
        vkAllocateCommandBuffers(device_->device(), &cmd_alloc_info, &command_buffer);

        VkCommandBufferBeginInfo begin_info{};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(command_buffer, &begin_info);

        // Transition to TRANSFER_DST_OPTIMAL
        VkImageMemoryBarrier barrier{};
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           = default_white_texture_->handle();
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount     = 1;
        barrier.srcAccessMask                   = 0;
        barrier.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
                             command_buffer,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &barrier);

        // Copy buffer to image
        VkBufferImageCopy region{};
        region.bufferOffset                    = 0;
        region.bufferRowLength                 = 0;
        region.bufferImageHeight               = 0;
        region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel       = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount     = 1;
        region.imageOffset                     = {0, 0, 0};
        region.imageExtent                     = {1, 1, 1};

        vkCmdCopyBufferToImage(
                               command_buffer,
                               staging_buffer,
                               default_white_texture_->handle(),
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &region);

        // Transition to SHADER_READ_ONLY_OPTIMAL
        barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
                             command_buffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0,
                             nullptr,
                             0,
                             nullptr,
                             1,
                             &barrier);

        vkEndCommandBuffer(command_buffer);

        // Submit
        VkSubmitInfo submit_info{};
        submit_info.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.commandBufferCount = 1;
        submit_info.pCommandBuffers    = &command_buffer;

        vkQueueSubmit(device_->graphics_queue(), 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(device_->graphics_queue());

        // Cleanup
        vkFreeCommandBuffers(device_->device(), command_pool, 1, &command_buffer);
        vkDestroyCommandPool(device_->device(), command_pool, nullptr);
        vkFreeMemory(device_->device(), staging_memory, nullptr);
        vkDestroyBuffer(device_->device(), staging_buffer, nullptr);

        default_white_texture_view_ = default_white_texture_->view();

        logger::info("Created default white texture for material: " + config_.name);
    }

    void Material::update_descriptor_set()
    {
        if (!uniform_buffer_ || descriptor_set_ == VK_NULL_HANDLE)
        {
            return;
        }

        // Update uniform buffer binding
        VkDescriptorBufferInfo buffer_info{};
        buffer_info.buffer = uniform_buffer_->buffer(0);
        buffer_info.offset = 0;
        buffer_info.range  = sizeof(UniformBufferData);

        VkWriteDescriptorSet descriptor_write{};
        descriptor_write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstSet          = descriptor_set_;
        descriptor_write.dstBinding      = 0;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pBufferInfo     = &buffer_info;

        vkUpdateDescriptorSets(device_->device(), 1, &descriptor_write, 0, nullptr);

        // Update texture bindings - always bind something to avoid validation errors

        for (uint32_t binding_slot = 1; binding_slot <= 4; ++binding_slot)

        {
            VkImageView view = default_white_texture_view_;

            VkSampler sampler = default_sampler_;
            if (sampler == VK_NULL_HANDLE)
            {
                logger::error("Default sampler is null for binding " + std::to_string(binding_slot));
                continue;
            }

            // Check if we have a texture for this binding slot

            for (const auto& [name, binding] : textures_)

            {
                if (binding.binding == binding_slot && binding.image && binding.view != VK_NULL_HANDLE)

                {
                    view = binding.view;

                    break;
                }
            }


            VkDescriptorImageInfo image_info{};
            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_info.imageView   = view;
            image_info.sampler     = sampler;

            VkWriteDescriptorSet texture_write{};

            texture_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

            texture_write.dstSet = descriptor_set_;

            texture_write.dstBinding = binding_slot;

            texture_write.dstArrayElement = 0;

            texture_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

            texture_write.descriptorCount = 1;

            texture_write.pImageInfo = &image_info;


            vkUpdateDescriptorSets(device_->device(), 1, &texture_write, 0, nullptr);
        }
    }
} // namespace vulkan_engine::rendering
#include "engine/rendering/resources/TextureLoader.hpp"
#include "engine/core/utils/Logger.hpp"
#include "engine/platform/filesystem/PathUtils.hpp"


#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <fstream>
#include <cstring>
#include <vector>
#include <filesystem>

namespace vulkan_engine::rendering
{
    TextureLoader::TextureLoader(std::shared_ptr<vulkan::DeviceManager> device)
        : device_(std::move(device))
    {
    }

    std::shared_ptr<vulkan::Image> TextureLoader::load_texture(
        const std::string& path,
        bool               generate_mipmaps)
    {
        return load_texture(path, VK_FORMAT_R8G8B8A8_UNORM, generate_mipmaps);
    }

    std::shared_ptr<vulkan::Image> TextureLoader::load_texture(
        const std::string& path,
        VkFormat           format,
        bool               generate_mipmaps)
    {
        (void)format; // Currently always uses VK_FORMAT_R8G8B8A8_UNORM
        std::string full_path = resolve_path(path);

        // Load file into memory first (handles Unicode paths correctly)
        auto file = core::PathUtils::open_input_file(full_path, std::ios::binary | std::ios::ate);
        if (!file.is_open())
        {
            logger::error("Failed to open texture file: " + core::PathUtils::to_string(std::filesystem::path(full_path)));
            return nullptr;
        }

        std::streamsize file_size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<uint8_t> buffer(file_size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), file_size))
        {
            logger::error("Failed to read texture file: " + core::PathUtils::to_string(std::filesystem::path(full_path)));
            return nullptr;
        }
        file.close();

        // Load image from memory using stb_image
        int      width, height, channels;
        stbi_uc* pixels = stbi_load_from_memory(buffer.data(), static_cast<int>(buffer.size()), &width, &height, &channels, STBI_rgb_alpha);

        if (!pixels)
        {
            logger::error("Failed to load texture: " + std::string(stbi_failure_reason()) + " - " +
                          core::PathUtils::to_string(std::filesystem::path(full_path)));
            return nullptr;
        }

        logger::info("Loaded texture: " + core::PathUtils::to_string(std::filesystem::path(full_path)) + " (" + std::to_string(width) + "x" +
                     std::to_string(height) + ", " + std::to_string(channels) +
                     " channels)");

        // Create GPU image
        auto image = create_image_from_data(pixels, width, height, 4, generate_mipmaps);

        // Free stb_image data
        stbi_image_free(pixels);

        return image;
    }

    std::string TextureLoader::resolve_path(const std::string& path) const
    {
        // 直接写死路径
        std::string full_path = "D:/TechArt/Vulkan/Textures/" + path;

        if (std::filesystem::exists(full_path))
        {
            return full_path;
        }

        // Fallback: try path as-is
        if (std::filesystem::exists(path))
        {
            return path;
        }

        return full_path;
    }

    std::shared_ptr<vulkan::Image> TextureLoader::create_image_from_data(
        const void* pixel_data,
        uint32_t    width,
        uint32_t    height,
        uint32_t    channels,
        bool        generate_mipmaps)
    {
        uint32_t mip_levels = generate_mipmaps ? calculate_mip_levels(width, height) : 1;

        // Create staging buffer
        VkDeviceSize image_size = width * height * channels;

        VkBuffer       staging_buffer;
        VkDeviceMemory staging_memory;

        VkBufferCreateInfo buffer_info{};
        buffer_info.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size        = image_size;
        buffer_info.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device_->device(), &buffer_info, nullptr, &staging_buffer) != VK_SUCCESS)
        {
            logger::error("Failed to create staging buffer for texture");
            return nullptr;
        }

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(device_->device(), staging_buffer, &mem_requirements);

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize  = mem_requirements.size;
        alloc_info.memoryTypeIndex = device_->find_memory_type(
                                                               mem_requirements.memoryTypeBits,
                                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        if (vkAllocateMemory(device_->device(), &alloc_info, nullptr, &staging_memory) != VK_SUCCESS)
        {
            vkDestroyBuffer(device_->device(), staging_buffer, nullptr);
            logger::error("Failed to allocate staging memory for texture");
            return nullptr;
        }

        vkBindBufferMemory(device_->device(), staging_buffer, staging_memory, 0);

        // Copy pixel data to staging buffer
        void* data;
        vkMapMemory(device_->device(), staging_memory, 0, image_size, 0, &data);
        memcpy(data, pixel_data, static_cast<size_t>(image_size));
        vkUnmapMemory(device_->device(), staging_memory);

        // Create GPU image
        VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        if (generate_mipmaps)
        {
            usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        auto image = std::make_shared<vulkan::Image>(
                                                     device_,
                                                     width,
                                                     height,
                                                     VK_FORMAT_R8G8B8A8_UNORM,
                                                     usage,
                                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                     mip_levels,
                                                     1);

        // Create one-time command buffer for layout transitions and copy
        VkCommandPool           command_pool;
        VkCommandPoolCreateInfo pool_info{};
        pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_info.queueFamilyIndex = 0; // Graphics queue
        pool_info.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        vkCreateCommandPool(device_->device(), &pool_info, nullptr, &command_pool);

        VkCommandBuffer             command_buffer;
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

        // Transition image layout to TRANSFER_DST_OPTIMAL
        VkImageMemoryBarrier barrier{};
        barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barrier.image                           = image->handle();
        barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel   = 0;
        barrier.subresourceRange.levelCount     = mip_levels;
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
        region.imageExtent                     = {width, height, 1};

        vkCmdCopyBufferToImage(
                               command_buffer,
                               staging_buffer,
                               image->handle(),
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &region);

        // Generate mipmaps if requested
        if (generate_mipmaps && mip_levels > 1)
        {
            int32_t mip_width  = width;
            int32_t mip_height = height;

            for (uint32_t i = 1; i < mip_levels; i++)
            {
                // Transition previous mip level to TRANSFER_SRC_OPTIMAL
                barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.subresourceRange.levelCount   = 1; // Only this level
                barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

                vkCmdPipelineBarrier(
                                     command_buffer,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     0,
                                     0,
                                     nullptr,
                                     0,
                                     nullptr,
                                     1,
                                     &barrier);

                // Blit from previous level to current level
                VkImageBlit blit{};
                blit.srcOffsets[0]                 = {0, 0, 0};
                blit.srcOffsets[1]                 = {mip_width, mip_height, 1};
                blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.mipLevel       = i - 1;
                blit.srcSubresource.baseArrayLayer = 0;
                blit.srcSubresource.layerCount     = 1;
                blit.dstOffsets[0]                 = {0, 0, 0};
                blit.dstOffsets[1]                 = {mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1};
                blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.dstSubresource.mipLevel       = i;
                blit.dstSubresource.baseArrayLayer = 0;
                blit.dstSubresource.layerCount     = 1;

                vkCmdBlitImage(
                               command_buffer,
                               image->handle(),
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               image->handle(),
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1,
                               &blit,
                               VK_FILTER_LINEAR);

                // Transition previous level to SHADER_READ_ONLY_OPTIMAL
                barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.subresourceRange.levelCount   = 1; // Only this level
                barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

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

                if (mip_width > 1) mip_width /= 2;
                if (mip_height > 1) mip_height /= 2;
            }

            // Transition last mip level
            barrier.subresourceRange.baseMipLevel = mip_levels - 1;
            barrier.subresourceRange.levelCount   = 1; // Only this level
            barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

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
        }
        else
        {
            // Transition to SHADER_READ_ONLY_OPTIMAL
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount   = mip_levels;
            barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

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
        }

        vkEndCommandBuffer(command_buffer);

        // Submit command buffer
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

        // Update image layout tracking (TextureLoader already emitted barriers)
        image->set_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        logger::info("Created GPU texture with " + std::to_string(mip_levels) + " mip levels");

        return image;
    }

    uint32_t TextureLoader::calculate_mip_levels(uint32_t width, uint32_t height) const
    {
        uint32_t max_dim = (width > height) ? width : height;
        uint32_t levels  = 0;
        while (max_dim > 0)
        {
            max_dim >>= 1;
            levels++;
        }
        return levels;
    }
} // namespace vulkan_engine::rendering
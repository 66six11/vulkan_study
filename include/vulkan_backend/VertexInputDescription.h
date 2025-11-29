//
// Created by C66 on 2025/11/30.
//
#pragma once
#ifndef VULKAN_VERTEXINPUTDESCRIPTION_H
#define VULKAN_VERTEXINPUTDESCRIPTION_H

#include <vulkan/vulkan.h>
#include <array>
#include "renderer/Vertex.h"

namespace vkvertex
{
    inline VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription desc{};
        desc.binding   = 0;              // 顶点缓冲绑定槽
        desc.stride    = sizeof(Vertex); // 每个顶点占多少字节
        desc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return desc;
    }

    inline std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions()
    {
        std::array<VkVertexInputAttributeDescription, 4> attrs{};

        // location 0: position (vec3)
        attrs[0].binding  = 0;
        attrs[0].location = 0;
        attrs[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[0].offset   = offsetof(Vertex, position);

        // location 1: normal (vec3)
        attrs[1].binding  = 0;
        attrs[1].location = 1;
        attrs[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[1].offset   = offsetof(Vertex, normal);

        // location 2: uv (vec2)
        attrs[2].binding  = 0;
        attrs[2].location = 2;
        attrs[2].format   = VK_FORMAT_R32G32_SFLOAT;
        attrs[2].offset   = offsetof(Vertex, uv);

        // location 3: color (vec4)
        attrs[3].binding  = 0;
        attrs[3].location = 3;
        attrs[3].format   = VK_FORMAT_R32G32B32A32_SFLOAT;
        attrs[3].offset   = offsetof(Vertex, color);


        return attrs;
    }
}
#endif //VULKAN_VERTEXINPUTDESCRIPTION_H

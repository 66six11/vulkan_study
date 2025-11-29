//
// Created by C66 on 2025/11/29.
//
#pragma once
#ifndef VULKAN_VERTEX_H
#define VULKAN_VERTEX_H

#include <glm/glm.hpp>


struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 color;
};

#endif //VULKAN_VERTEX_H

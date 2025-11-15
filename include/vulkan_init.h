#pragma once

#include "HelloTriangleApplication.h"

// Vulkan初始化相关函数
void HelloTriangleApplication::initWindow();
void HelloTriangleApplication::initVulkan();
void HelloTriangleApplication::createInstance();
void HelloTriangleApplication::setupDebugMessenger();
void HelloTriangleApplication::createSurface();
void HelloTriangleApplication::pickPhysicalDevice();
void HelloTriangleApplication::createLogicalDevice();
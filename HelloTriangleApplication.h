// HelloTriangleApplication.h
#pragma once // 防止头文件被多次包含

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <cstdlib>

// 声明常量（外部链接）
extern const uint32_t WIDTH;
extern const uint32_t HEIGHT;

class HelloTriangleApplication {
public:
    void run();

private:

    GLFWwindow *window = nullptr;

    VkInstance instance = VK_NULL_HANDLE;

    void initWindow();

    void initVulkan();

    void mainLoop();

    void cleanup();

    void createInstance();
};

#include "../include/HelloTriangleApplication.h"
#include "../include/vulkan_init.h"
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <optional>
#include <set>
#include <cstdint>
#include <algorithm>
#include <limits>

// 初始化GLFW窗口
// 设置窗口属性并创建窗口
void HelloTriangleApplication::initWindow() {
    // 初始化GLFW库
    glfwInit();

    // 设置GLFW不创建OpenGL上下文（因为我们要使用Vulkan）
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // 设置窗口不可调整大小
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    // 创建窗口，指定尺寸、标题和其它参数
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
}

// 初始化Vulkan
// 按顺序创建所有必需的Vulkan对象
void HelloTriangleApplication::initVulkan() {
    // 创建Vulkan实例，这是与Vulkan驱动程序交互的入口点
    createInstance();
    // 设置调试信息回调（可选功能，用于开发时调试）
    setupDebugMessenger(); // 可选的调试功能
    // 创建窗口表面，用于连接Vulkan与窗口系统
    createSurface();
    // 选择合适的物理设备（GPU）
    pickPhysicalDevice();
    // 创建逻辑设备，用于与物理设备进行交互
    createLogicalDevice();
}

// 创建Vulkan实例
// Vulkan实例是与Vulkan驱动程序交互的入口点
void HelloTriangleApplication::createInstance() {
    // 填充应用程序信息结构体
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;  // 结构体类型标识
    appInfo.pApplicationName = "Hello Triangle";         // 应用程序名称
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);  // 应用程序版本
    appInfo.pEngineName = "No Engine";                   // 引擎名称
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);    // 引擎版本
    appInfo.apiVersion = VK_API_VERSION_1_0;             // 使用的Vulkan API版本

    // 填充实例创建信息结构体
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;  // 结构体类型标识
    createInfo.pApplicationInfo = &appInfo;                     // 指向应用程序信息

    // 获取GLFW所需的扩展
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // 启用GLFW所需的扩展
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;

    // 不启用任何验证层（在生产环境中可能需要启用）
    createInfo.enabledLayerCount = 0;


    // 打印Vulkan版本信息和可用扩展（调试信息）
    std::cout << "Vulkan Version: " << VK_VERSION_1_0 << std::endl;
    std::cout << "Available Extensions:" << createInfo.enabledExtensionCount << std::endl;
    for (uint32_t i = 0; i < createInfo.enabledExtensionCount; i++) {
        std::cout << "	" << createInfo.ppEnabledExtensionNames[i] << std::endl;
    }


    // 创建Vulkan实例
    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

// 设置调试信息回调
// 在这个基础示例中留空，但在开发过程中可以实现调试回调来捕获Vulkan错误和警告
void HelloTriangleApplication::setupDebugMessenger() {
    // 这里留空，因为调试功能在基础示例中可选
    // 如果需要调试功能，可以实现调试回调
}

// 创建窗口表面
// 创建Vulkan与窗口系统之间的接口，允许将渲染结果呈现到窗口
// Vulkan与窗口系统无关，需要使用WSI（Window System Integration）扩展
void HelloTriangleApplication::createSurface() {
    // 使用GLFW创建窗口表面，这是窗口系统与Vulkan之间的接口
    // Vulkan与窗口系统无关，需要使用WSI（Window System Integration）扩展
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

// 选择物理设备（GPU）
// 枚举所有支持Vulkan的物理设备并选择最合适的设备
void HelloTriangleApplication::pickPhysicalDevice() {
    // 枚举可用的物理设备数量
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    // 检查是否有支持Vulkan的设备
    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    // 获取所有物理设备
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    // 遍历设备并查找合适的设备
    for (const auto& device : devices) {
        // 检查设备是否适合（支持图形队列、交换链扩展等）
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    // 确保找到了合适的设备
    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

// 创建逻辑设备
// 创建与物理设备关联的逻辑设备，用于执行图形操作
void HelloTriangleApplication::createLogicalDevice() {
    // 找到支持图形的队列族
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    // 准备队列创建信息
    // 使用set确保队列族索引唯一，避免重复创建
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    // 设置队列优先级（范围0.0-1.0，1.0为最高优先级）
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;  // 结构体类型标识
        queueCreateInfo.queueFamilyIndex = queueFamily;  // 队列族索引
        queueCreateInfo.queueCount = 1;  // 创建一个队列
        queueCreateInfo.pQueuePriorities = &queuePriority;  // 队列优先级
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // 设置设备功能（这里不启用任何特殊功能）
    VkPhysicalDeviceFeatures deviceFeatures{};

    // 创建逻辑设备
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;  // 结构体类型标识
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());  // 队列创建信息数量
    createInfo.pQueueCreateInfos = queueCreateInfos.data();  // 指向队列创建信息数组
    createInfo.pEnabledFeatures = &deviceFeatures;  // 指向设备功能结构体
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());  // 启用的扩展数量
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();  // 指向启用的扩展名称数组

    // 旧版本兼容性：启用层（在新版本中与实例层分离）
    createInfo.enabledLayerCount = 0;

    // 创建逻辑设备
    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    // 获取队列句柄，用于后续向队列提交命令
    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);  // 获取图形队列
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);   // 获取呈现队列
}
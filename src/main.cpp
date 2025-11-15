#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>

#include "Application.h"

/**
 * @brief 程序入口点
 * 
 * Vulkan应用程序的主入口函数，负责创建Application实例并运行应用程序
 * 
 * @return 程序退出状态码
 */
int main() {
    // 使用try-catch块捕获Vulkan可能抛出的异常
    try {
        // 创建应用程序实例
        Application app;
        // 运行应用程序的主要逻辑
        app.run();
    } catch (const std::exception &e) {
        // 捕获并打印异常信息
        std::cerr << e.what() << std::endl;
        // 返回失败状态码
        return EXIT_FAILURE;
    }

    // 程序正常退出
    return EXIT_SUCCESS;
}

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include "platform/Engine.h"

/**
 * @brief 程序入口点
 * 
 * Vulkan应用程序的主入口函数，负责创建Application实例并运行应用程序
 * 
 * @return 程序退出状态码
 */
int main()
{
    // 使用try-catch块捕获Vulkan可能抛出的异常
    try
    {
        Engine app;
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    // 程序正常退出
    return EXIT_SUCCESS;
}
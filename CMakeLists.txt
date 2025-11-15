cmake_minimum_required(VERSION 3.20)
project(vulkan LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 设置Vulkan SDK路径
if(DEFINED ENV{VULKAN_SDK})
    set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
else()
    set(VULKAN_SDK_PATH "C:/VulkanSDK/1.4.321.1")
endif()

# 尝试使用find_package查找Vulkan
find_package(Vulkan REQUIRED)
if(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK not found. Please install Vulkan SDK from LunarG.")
endif()

# 设置vcpkg工具链文件路径
set(VCPKG_ROOT_PATH "C:/Users/C66/.vcpkg-clion/vcpkg")
if(EXISTS "${VCPKG_ROOT_PATH}/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT_PATH}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Vcpkg toolchain file")
    set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "Vcpkg target triplet")
endif()

# 查找GLFW
find_package(glfw3 REQUIRED CONFIG)
if(NOT glfw3_FOUND)
    message(FATAL_ERROR "GLFW not found. Please install GLFW via vcpkg: vcpkg install glfw3:x64-windows")
endif()

# 添加可执行文件
add_executable(vulkan
    src/main.cpp
    src/HelloTriangleApplication.cpp
    src/vulkan_init.cpp
    src/swapchain_management.cpp
    src/rendering.cpp
    src/command_buffer_sync.cpp
    src/utils.cpp
    include/HelloTriangleApplication.h
    include/vulkan_init.h
    include/swapchain_management.h
    include/rendering.h
    include/command_buffer_sync.h
    include/utils.h
)

# 链接库
target_link_libraries(vulkan PRIVATE 
    Vulkan::Vulkan
    glfw
)

# 包含目录
target_include_directories(vulkan PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
)

# 设置Visual Studio调试工作目录
set_target_properties(vulkan PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
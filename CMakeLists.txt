cmake_minimum_required(VERSION 3.20)
project(vulkan LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 查找Vulkan SDK
find_package(Vulkan REQUIRED)

if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK not found. Please install Vulkan SDK from LunarG.")
endif ()

# 查找GLFW

find_package(glfw3 CONFIG REQUIRED)


if (NOT glfw3_FOUND)

    message(FATAL_ERROR "GLFW3 not found. Please install via vcpkg or set up your CMake properly.")

endif ()

# 添加可执行文件


add_executable(vulkan
        src/main.cpp
        src/VulkanApp.cpp
        src/vulkan_init.cpp
        src/swapchain_management.cpp
        src/rendering.cpp
        src/command_buffer_sync.cpp
        src/utils.cpp
        src/constants.cpp
        include/Application.h
        include/constants.h
        include/vulkan_init.h
        include/swapchain_management.h
        include/rendering.h
        include/command_buffer_sync.h
        include/utils.h
        include/Platform.h
        src/utils.cpp
        src/utils.cpp
        include/SwapchainResources.h
        src/SwapchainResources.cpp
        src/VulkanDevice.cpp
        include/VulkanDevice.h
        include/ResourceManager.h
        include/ResourceManager.h
        include/DescriptorSetmanager.h
        include/DescriptorSetmanager.h
        src/ResourceManager.cpp
        src/DescriptorSetManager.cpp
        include/Renderer.h
        include/VulkanRenderer.h
        src/VulkanRenderer.cpp
        include/TimeStamp.h

)

# 链接库
target_link_libraries(vulkan PRIVATE
        Vulkan::Vulkan
)

# 链接GLFW库

target_link_libraries(vulkan PRIVATE glfw)

# 包含目录
target_include_directories(vulkan PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${Vulkan_INCLUDE_DIRS}
)

# 如果找到了GLFW包含目录
if (GLFW_INCLUDE_DIRS)
    target_include_directories(vulkan PRIVATE ${GLFW_INCLUDE_DIRS})
endif ()

# 设置Visual Studio调试工作目录为可执行文件目录
# 这样运行时可以正确找到相对路径的着色器文件
set_target_properties(vulkan PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:vulkan>
)


# 查找GLSL编译器

find_program(GLSLC glslc)

if (NOT GLSLC)

    message(WARNING "glslc not found. Shaders will not be compiled automatically.")

else ()

    # 编译着色器

    set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)

    set(COMPILED_SHADER_DIR ${CMAKE_BINARY_DIR}/shaders)

    file(MAKE_DIRECTORY ${COMPILED_SHADER_DIR})


    # 顶点着色器编译规则

    add_custom_command(

            OUTPUT ${COMPILED_SHADER_DIR}/shader.vert.spv

            COMMAND ${GLSLC} -o ${COMPILED_SHADER_DIR}/shader.vert.spv ${SHADER_DIR}/shader.vert

            DEPENDS ${SHADER_DIR}/shader.vert

            COMMENT "Compiling vertex shader"

    )


    # 片段着色器编译规则

    add_custom_command(

            OUTPUT ${COMPILED_SHADER_DIR}/shader.frag.spv

            COMMAND ${GLSLC} -o ${COMPILED_SHADER_DIR}/shader.frag.spv ${SHADER_DIR}/shader.frag

            DEPENDS ${SHADER_DIR}/shader.frag

            COMMENT "Compiling fragment shader"

    )


    # 创建着色器目标

    add_custom_target(compile_shaders

            DEPENDS ${COMPILED_SHADER_DIR}/shader.vert.spv ${COMPILED_SHADER_DIR}/shader.frag.spv

    )
    add_dependencies(vulkan compile_shaders)


    # 设置输出目录为运行时目录
    # 复制编译后的着色器到可执行文件目录
    add_custom_command(TARGET vulkan POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${COMPILED_SHADER_DIR} $<TARGET_FILE_DIR:vulkan>/shaders
    )

    # 同时复制到源码根目录，以支持从源码目录运行
    add_custom_command(TARGET vulkan POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${COMPILED_SHADER_DIR} ${CMAKE_SOURCE_DIR}/shaders
    )

endif ()
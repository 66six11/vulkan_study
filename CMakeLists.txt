cmake_minimum_required(VERSION 3.20)
project(vulkan LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 查找Vulkan SDK
find_package(Vulkan REQUIRED)

if(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan SDK not found. Please install Vulkan SDK from LunarG.")
endif()

# 尝试查找GLFW
find_package(glfw3 QUIET)

if(NOT glfw3_FOUND)
    # 如果未找到glfw3包，尝试使用pkg-config
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(GLFW QUIET glfw3)
    endif()
    
    # 如果仍未找到，设置默认路径（需要根据实际安装路径修改）
    if(NOT GLFW_FOUND)
        message(WARNING "GLFW not found via find_package or pkg-config. You may need to set GLFW paths manually.")
        # 设置GLFW的包含目录和库文件路径（请根据实际安装路径修改）
        # set(GLFW_INCLUDE_DIRS "C:/path/to/glfw/include")
        # set(GLFW_LIBRARIES "C:/path/to/glfw/lib/glfw3.lib")
    endif()
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
    include/Application.h
    include/constants.h
    include/vulkan_init.h
    include/swapchain_management.h
    include/rendering.h
    include/command_buffer_sync.h
    include/utils.h
)

# 链接库
target_link_libraries(vulkan PRIVATE 
    Vulkan::Vulkan
)

# 如果找到了glfw3包
if(TARGET glfw)
    target_link_libraries(vulkan PRIVATE glfw)
elseif(GLFW_LIBRARIES)
    target_link_libraries(vulkan PRIVATE ${GLFW_LIBRARIES})
else()
    message(WARNING "GLFW library not found. You may need to manually specify the GLFW library path.")
endif()

# 包含目录
target_include_directories(vulkan PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
)

# 如果找到了GLFW包含目录
if(GLFW_INCLUDE_DIRS)
    target_include_directories(vulkan PRIVATE ${GLFW_INCLUDE_DIRS})
endif()

# 设置Visual Studio调试工作目录

set_target_properties(vulkan PROPERTIES

    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}

)



# 查找GLSL编译器

find_program(GLSLC glslc)

if(NOT GLSLC)

    message(WARNING "glslc not found. Shaders will not be compiled automatically.")

else()

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



    # 设置输出目录为运行时目录

    add_custom_command(TARGET vulkan POST_BUILD

        COMMAND ${CMAKE_COMMAND} -E copy_directory

        ${COMPILED_SHADER_DIR} $<TARGET_FILE_DIR:vulkan>/shaders

    )

endif()
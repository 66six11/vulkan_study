# 编译器标志集中管理

# 通用警告标志
set(VULKAN_ENGINE_WARNINGS
        /W4            # MSVC: 警告级别4
        /permissive-   # MSVC: 严格标准符合性
)

# MSVC特定标志
if (MSVC)
    list(APPEND VULKAN_ENGINE_WARNINGS
            /WX               # 将警告视为错误
            /utf-8            # 使用UTF-8源文件编码
    )
endif ()

# GCC/Clang警告标志
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(VULKAN_ENGINE_WARNINGS
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wsign-conversion
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Wcast-align
            -Wunused
            -Woverloaded-virtual
            -Wpedantic
    )
endif ()

# Clang额外警告
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    list(APPEND VULKAN_ENGINE_WARNINGS
            -Weverything
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-missing-prototypes
            -Wno-padded
            -Wno-undef
    )
endif ()

# GCC额外警告
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    list(APPEND VULKAN_ENGINE_WARNINGS
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wnull-dereference
    )
endif ()

# 调试标志
if (MSVC)
    set(VULKAN_ENGINE_DEBUG_FLAGS
            /Zi          # 生成调试信息
            /Od          # 禁用优化
            /RTC1        # 运行时错误检查
    )
else ()
    set(VULKAN_ENGINE_DEBUG_FLAGS
            -g
            -fno-omit-frame-pointer
    )
endif ()

# 发布优化标志
set(VULKAN_ENGINE_RELEASE_FLAGS
        -O3
        -DNDEBUG
)

if (MSVC)
    set(VULKAN_ENGINE_RELEASE_FLAGS
            /O2
            /GL
            /DNDEBUG
    )
endif ()

# 链接时优化标志
set(VULKAN_ENGINE_LTO_FLAGS
        -flto
        -fno-fat-lto-objects
)

if (MSVC)
    set(VULKAN_ENGINE_LTO_FLAGS
            /LTCG
    )
endif ()

# Sanitizer标志
set(VULKAN_ENGINE_SANITIZER_FLAGS
        -fsanitize=address
        -fsanitize=undefined
        -fno-sanitize-recover=all
)

# 应用编译器标志到目标
function(apply_compiler_flags TARGET)
    # 检测目标类型
    get_target_property(TARGET_TYPE ${TARGET} TYPE)

    # INTERFACE 目标必须使用 INTERFACE 关键字
    if (TARGET_TYPE STREQUAL "INTERFACE_LIBRARY")
        set(KEYWORD INTERFACE)
    else ()
        set(KEYWORD PRIVATE)
    endif ()

    target_compile_options(${TARGET} ${KEYWORD} ${VULKAN_ENGINE_WARNINGS})

    # 应用调试/发布标志
    target_compile_options(${TARGET} ${KEYWORD}
            $<$<CONFIG:Debug>:${VULKAN_ENGINE_DEBUG_FLAGS}>
            $<$<CONFIG:Release>:${VULKAN_ENGINE_RELEASE_FLAGS}>
    )

    # 应用LTO (仅非INTERFACE目标)
    if (VULKAN_ENGINE_ENABLE_LTO AND NOT TARGET_TYPE STREQUAL "INTERFACE_LIBRARY")
        target_compile_options(${TARGET} ${KEYWORD} ${VULKAN_ENGINE_LTO_FLAGS})
        if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_link_options(${TARGET} ${KEYWORD} ${VULKAN_ENGINE_LTO_FLAGS})
        endif ()
    endif ()

    # 应用Sanitizers (仅非INTERFACE目标)
    if (VULKAN_ENGINE_ENABLE_SANITIZERS AND NOT TARGET_TYPE STREQUAL "INTERFACE_LIBRARY")
        target_compile_options(${TARGET} ${KEYWORD} ${VULKAN_ENGINE_SANITIZER_FLAGS})
        target_link_options(${TARGET} ${KEYWORD} ${VULKAN_ENGINE_SANITIZER_FLAGS})
    endif ()
endfunction()

# 启用静态分析工具
function(enable_static_analysis TARGET)
    if (VULKAN_ENGINE_ENABLE_CLANG_TIDY AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        find_program(CLANG_TIDY_EXE NAMES clang-tidy)
        if (CLANG_TIDY_EXE)
            set_target_properties(${TARGET} PROPERTIES
                    CXX_CLANG_TIDY "${CLANG_TIDY_EXE}"
            )
            message(STATUS "Enabled clang-tidy for ${TARGET}")
        endif ()
    endif ()

    if (VULKAN_ENGINE_ENABLE_CPPCHECK)
        find_program(CPPCHECK_EXE NAMES cppcheck)
        if (CPPCHECK_EXE)
            set_target_properties(${TARGET} PROPERTIES
                    CXX_CPPCHECK "${CPPCHECK_EXE}"
            )
            message(STATUS "Enabled cppcheck for ${TARGET}")
        endif ()
    endif ()
endfunction()

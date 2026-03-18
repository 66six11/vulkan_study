# 测试模块 CMake 配置

# 自动发现测试源文件
file(GLOB_RECURSE GTEST_SOURCES "vulkan/*Test.cpp")
file(GLOB_RECURSE SIMPLE_TEST_SOURCES "vulkan/*Main.cpp")

# 过滤掉不存在的文件（CMake 缓存可能包含已删除的文件）
set(EXISTING_GTEST_SOURCES)
foreach (src ${GTEST_SOURCES})
    if (EXISTS ${src})
        list(APPEND EXISTING_GTEST_SOURCES ${src})
    else ()
        message(WARNING "Test source file does not exist, skipping: ${src}")
    endif ()
endforeach ()
set(GTEST_SOURCES ${EXISTING_GTEST_SOURCES})

message(STATUS "Found GTest sources: ${GTEST_SOURCES}")
message(STATUS "Found simple test sources: ${SIMPLE_TEST_SOURCES}")

# 查找 GTest 库文件
file(GLOB GTEST_LIB_DIRS "$ENV{USERPROFILE}/.conan2/p/b/gtest*/p/lib")
file(GLOB GTEST_INCLUDE_DIRS "$ENV{USERPROFILE}/.conan2/p/b/gtest*/p/include")

if (GTEST_LIB_DIRS AND GTEST_INCLUDE_DIRS)
    # 区分 Debug 和 Release 版本的库目录（Debug 版本库文件通常更大）
    set(GTEST_DEBUG_DIR "")
    set(GTEST_RELEASE_DIR "")
    set(GTEST_DEBUG_SIZE 0)
    set(GTEST_RELEASE_SIZE 0)

    foreach (lib_dir ${GTEST_LIB_DIRS})
        set(gtest_lib "${lib_dir}/gtest.lib")
        if (EXISTS ${gtest_lib})
            file(SIZE ${gtest_lib} lib_size)
            # Debug 版本的库文件通常大于 5MB，Release 版本通常小于 5MB
            if (lib_size GREATER 5242880)  # 5MB = 5 * 1024 * 1024
                if (lib_size GREATER GTEST_DEBUG_SIZE)
                    set(GTEST_DEBUG_SIZE ${lib_size})
                    set(GTEST_DEBUG_DIR ${lib_dir})
                endif ()
            else ()
                if (lib_size GREATER GTEST_RELEASE_SIZE)
                    set(GTEST_RELEASE_SIZE ${lib_size})
                    set(GTEST_RELEASE_DIR ${lib_dir})
                endif ()
            endif ()
        endif ()
    endforeach ()

    # 优先使用 Debug 目录的 include
    if (GTEST_DEBUG_DIR)
        string(REPLACE "/lib" "/include" GTEST_INCLUDE_DIR ${GTEST_DEBUG_DIR})
    else ()
        list(GET GTEST_INCLUDE_DIRS 0 GTEST_INCLUDE_DIR)
    endif ()

    message(STATUS "GTest Debug dir: ${GTEST_DEBUG_DIR} (${GTEST_DEBUG_SIZE} bytes)")
    message(STATUS "GTest Release dir: ${GTEST_RELEASE_DIR} (${GTEST_RELEASE_SIZE} bytes)")

    # 创建 GTest 导入目标
    add_library(GTest::gtest STATIC IMPORTED)
    set_target_properties(GTest::gtest PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${GTEST_INCLUDE_DIR}"
    )

    # 设置 Debug 和 Release 的库路径（同时设置 MinSizeRel 和 RelWithDebInfo）
    if (GTEST_DEBUG_DIR AND GTEST_RELEASE_DIR)
        set_target_properties(GTest::gtest PROPERTIES
                IMPORTED_LOCATION_DEBUG "${GTEST_DEBUG_DIR}/gtest.lib"
                IMPORTED_LOCATION_RELEASE "${GTEST_RELEASE_DIR}/gtest.lib"
                IMPORTED_LOCATION_MINSIZEREL "${GTEST_RELEASE_DIR}/gtest.lib"
                IMPORTED_LOCATION_RELWITHDEBINFO "${GTEST_RELEASE_DIR}/gtest.lib"
        )
    elseif (GTEST_DEBUG_DIR)
        set_target_properties(GTest::gtest PROPERTIES
                IMPORTED_LOCATION "${GTEST_DEBUG_DIR}/gtest.lib"
        )
    elseif (GTEST_RELEASE_DIR)
        set_target_properties(GTest::gtest PROPERTIES
                IMPORTED_LOCATION "${GTEST_RELEASE_DIR}/gtest.lib"
        )
    endif ()

    add_library(GTest::gtest_main STATIC IMPORTED)
    if (GTEST_DEBUG_DIR AND GTEST_RELEASE_DIR)
        set_target_properties(GTest::gtest_main PROPERTIES
                IMPORTED_LOCATION_DEBUG "${GTEST_DEBUG_DIR}/gtest_main.lib"
                IMPORTED_LOCATION_RELEASE "${GTEST_RELEASE_DIR}/gtest_main.lib"
                IMPORTED_LOCATION_MINSIZEREL "${GTEST_RELEASE_DIR}/gtest_main.lib"
                IMPORTED_LOCATION_RELWITHDEBINFO "${GTEST_RELEASE_DIR}/gtest_main.lib"
                INTERFACE_LINK_LIBRARIES "GTest::gtest"
        )
    elseif (GTEST_DEBUG_DIR)
        set_target_properties(GTest::gtest_main PROPERTIES
                IMPORTED_LOCATION "${GTEST_DEBUG_DIR}/gtest_main.lib"
                INTERFACE_LINK_LIBRARIES "GTest::gtest"
        )
    elseif (GTEST_RELEASE_DIR)
        set_target_properties(GTest::gtest_main PROPERTIES
                IMPORTED_LOCATION "${GTEST_RELEASE_DIR}/gtest_main.lib"
                INTERFACE_LINK_LIBRARIES "GTest::gtest"
        )
    endif ()

    set(GTest_FOUND TRUE)
    message(STATUS "GTest configured: Debug=${GTEST_DEBUG_DIR}, Release=${GTEST_RELEASE_DIR}")
else ()
    # 尝试使用 find_package
    find_package(GTest QUIET)
endif ()

if (GTest_FOUND AND GTEST_SOURCES)
    message(STATUS "GTest found, building GTest-based tests")

    # 如果 gtest 是 Release 版本但构建是 Debug，跳过测试以避免运行时库不匹配
    set(GTEST_RELEASE_ONLY FALSE)
    if (NOT GTEST_DEBUG_DIR)
        set(GTEST_RELEASE_ONLY TRUE)
        if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR "Debug" IN_LIST CMAKE_CONFIGURATION_TYPES)
            message(WARNING "GTest is Release-only but building in Debug mode. "
                    "Skipping GTest-based tests to avoid runtime library mismatch. "
                    "Rebuild GTest with Debug configuration or build tests in Release mode.")
            # 清空 GTEST_SOURCES 以跳过测试创建
            set(GTEST_SOURCES)
        endif ()
    endif ()

    # 为每个 GTest 源文件创建测试可执行文件
    foreach (test_source ${GTEST_SOURCES})
        # 获取文件名（不含路径和扩展名）
        get_filename_component(test_name ${test_source} NAME_WE)

        add_executable(${test_name} ${test_source})

        target_link_libraries(${test_name} PRIVATE
                VulkanEngineVulkan
                GTest::gtest_main
        )

        set_target_properties(${test_name} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests
                CXX_STANDARD 20
                CXX_STANDARD_REQUIRED ON
        )

        if (MSVC)
            fix_msvc_runtime_conflicts(${test_name})
            target_compile_options(${test_name} PRIVATE /FS)
        endif ()

        # 添加到测试列表
        list(APPEND GTEST_TARGETS ${test_name})
    endforeach ()

    # 使用 GTest 发现测试
    include(GoogleTest)
    foreach (test_target ${GTEST_TARGETS})
        gtest_discover_tests(${test_target}
                TEST_PREFIX "${test_target}."
                DISCOVERY_MODE PRE_TEST
        )
    endforeach ()

    # 添加运行所有 GTest 的目标
    if (GTEST_TARGETS)
        add_custom_target(run-all-gtests
                COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
                DEPENDS ${GTEST_TARGETS}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Running All GTests"
        )
    endif ()
else ()
    message(STATUS "GTest not found or no GTest sources, skipping GTest-based tests")
endif ()

# 简单测试框架版本（不依赖外部库）
if (SIMPLE_TEST_SOURCES)
    foreach (test_source ${SIMPLE_TEST_SOURCES})
        get_filename_component(test_name ${test_source} NAME_WE)

        add_executable(${test_name} ${test_source})

        target_link_libraries(${test_name} PRIVATE
                VulkanEngineVulkan
        )

        set_target_properties(${test_name} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests
                CXX_STANDARD 20
                CXX_STANDARD_REQUIRED ON
        )

        if (MSVC)
            fix_msvc_runtime_conflicts(${test_name})
            target_compile_options(${test_name} PRIVATE /FS)
        endif ()

        list(APPEND SIMPLE_TEST_TARGETS ${test_name})

        # 添加单个测试运行目标
        add_custom_target(run-${test_name}
                COMMAND ${test_name}
                DEPENDS ${test_name}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests
                COMMENT "Running ${test_name}"
        )
    endforeach ()

    # 添加运行所有简单测试的目标
    if (SIMPLE_TEST_TARGETS)
        add_custom_target(run-simple-tests
                COMMAND ${CMAKE_COMMAND} -E echo "Running all simple tests..."
                DEPENDS ${SIMPLE_TEST_TARGETS}
                COMMENT "Running All Simple Tests"
        )

        foreach (target ${SIMPLE_TEST_TARGETS})
            add_custom_command(TARGET run-simple-tests POST_BUILD
                    COMMAND ${target}
                    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tests
                    COMMENT "Running ${target}"
            )
        endforeach ()
    endif ()
else ()
    message(STATUS "No simple test sources found")
endif ()

# 添加运行所有测试的目标
if (GTEST_TARGETS OR SIMPLE_TEST_TARGETS)
    set(ALL_TEST_TARGETS ${GTEST_TARGETS} ${SIMPLE_TEST_TARGETS})
    add_custom_target(run-all-tests
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            DEPENDS ${ALL_TEST_TARGETS}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Running All Tests"
    )
endif ()

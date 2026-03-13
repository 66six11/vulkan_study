#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
CMake 模块化架构验证脚本
"""

import os
import sys
import subprocess
from pathlib import Path

def print_section(title):
    print(f"\n{'='*60}")
    print(f"  {title}")
    print(f"{'='*60}\n")

def check_file_exists(filepath, description):
    if os.path.exists(filepath):
        print(f"✓ {description}: {filepath}")
        return True
    else:
        print(f"✗ {description}: {filepath} (未找到)")
        return False

def check_directory_structure():
    print_section("检查目录结构")

    required_dirs = [
        ("cmake", "CMake配置目录"),
        ("cmake/Modules", "CMake模块目录"),
        ("src", "源代码目录"),
        ("include", "头文件目录"),
    ]

    all_ok = True
    for dirpath, description in required_dirs:
        if not check_file_exists(dirpath, description):
            all_ok = False

    return all_ok

def check_module_files():
    print_section("检查模块文件")

    required_files = [
        ("CMakeLists.txt", "主CMake文件"),
        ("cmake/Options.cmake", "选项配置"),
        ("cmake/CompilerFlags.cmake", "编译器标志"),
        ("cmake/Dependencies.cmake", "依赖管理"),
        ("cmake/Utils.cmake", "工具函数"),
        ("cmake/Modules/VulkanModule.cmake", "模块创建函数"),
        ("cmake/Modules/Core.cmake", "Core模块"),
        ("cmake/Modules/Platform.cmake", "Platform模块"),
        ("cmake/Modules/Rendering.cmake", "Rendering模块"),
        ("cmake/Modules/Vulkan.cmake", "Vulkan模块"),
        ("cmake/Modules/Application.cmake", "Application模块"),
    ]

    all_ok = True
    for filepath, description in required_files:
        if not check_file_exists(filepath, description):
            all_ok = False

    return all_ok

def check_source_files():
    print_section("检查源文件")

    modules = {
        "core": 0,  # 纯头文件模块
        "platform": 3,
        "rendering": 4,
        "vulkan": 5,
        "application": 2,
    }

    all_ok = True
    for module, expected_count in modules.items():
        src_dir = f"src/{module}"
        if not os.path.exists(src_dir):
            print(f"✗ {module} 源目录未找到: {src_dir}")
            all_ok = False
            continue

        cpp_files = list(Path(src_dir).rglob("*.cpp"))
        actual_count = len(cpp_files)

        if actual_count == expected_count:
            print(f"✓ {module}: {actual_count} 个源文件")
        else:
            print(f"⚠ {module}: 预期 {expected_count} 个源文件，实际 {actual_count} 个")
            if actual_count > 0:
                for cpp_file in cpp_files:
                    print(f"    - {cpp_file}")

    return all_ok

def check_conan_integration():
    print_section("检查Conan集成")

    conan_files = [
        ("conanfile.py", "Conan配置文件"),
        ("build/build/generators/conan_toolchain.cmake", "Conan工具链"),
    ]

    all_ok = True
    for filepath, description in conan_files:
        if not check_file_exists(filepath, description):
            all_ok = False

    return all_ok

def test_cmake_configuration():
    print_section("测试CMake配置")

    build_dir = "build_test"

    # 清理旧构建
    if os.path.exists(build_dir):
        print(f"清理旧构建目录: {build_dir}")
        subprocess.run(["rm", "-rf", build_dir], shell=True)

    # 创建构建目录
    os.makedirs(build_dir, exist_ok=True)
    print(f"创建构建目录: {build_dir}")

    # 运行CMake配置
    print("运行CMake配置...")
    result = subprocess.run(
        ["cmake", "-S", ".", "-B", build_dir, "-DCMAKE_BUILD_TYPE=Release"],
        capture_output=True,
        text=True
    )

    print("\nCMake 输出:")
    print("="*60)
    print(result.stdout)
    if result.stderr:
        print("CMake 错误:")
        print(result.stderr)
    print("="*60)

    if result.returncode != 0:
        print(f"\n✗ CMake 配置失败，返回码: {result.returncode}")
        return False

    print("\n✓ CMake 配置成功")
    return True

def main():
    print("\n" + "="*60)
    print("  CMake 模块化架构验证")
    print("="*60)

    checks = [
        ("目录结构", check_directory_structure),
        ("模块文件", check_module_files),
        ("源文件", check_source_files),
        ("Conan集成", check_conan_integration),
    ]

    results = []
    for check_name, check_func in checks:
        try:
            result = check_func()
            results.append((check_name, result))
        except Exception as e:
            print(f"\n✗ {check_name} 检查时出错: {e}")
            results.append((check_name, False))

    # 总结
    print_section("验证结果")
    for check_name, result in results:
        status = "✓ 通过" if result else "✗ 失败"
        print(f"{check_name:20s} {status}")

    # 如果所有检查都通过，测试CMake配置
    if all(result for _, result in results):
        print("\n所有基础检查通过，开始测试CMake配置...")
        cmake_result = test_cmake_configuration()
        results.append(("CMake配置", cmake_result))
    else:
        print("\n基础检查未通过，跳过CMake配置测试")

    # 最终总结
    print_section("最终总结")
    total = len(results)
    passed = sum(1 for _, result in results if result)

    print(f"通过: {passed}/{total}")

    if passed == total:
        print("\n✓ 所有检查通过，CMake模块化架构正确！")
        return 0
    else:
        print("\n✗ 部分检查失败，请查看上面的错误信息")
        return 1

if __name__ == "__main__":
    sys.exit(main())

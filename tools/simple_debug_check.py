#!/usr/bin/env python3
"""
简化版CLion调试环境检查
"""

import os
import sys
from pathlib import Path

def main():
    print("=" * 60)
    print("CLion调试环境快速检查")
    print("=" * 60)
    
    project_root = Path(__file__).parent.parent
    
    checks = []
    
    # 1. 检查Vulkan SDK
    vulkan_sdk = os.environ.get('VULKAN_SDK')
    if vulkan_sdk and Path(vulkan_sdk).exists():
        print("[OK] Vulkan SDK已安装:", vulkan_sdk)
        checks.append(True)
    else:
        print("[WARN] Vulkan SDK未设置或不存在")
        checks.append(False)
    
    # 2. 检查构建目录
    debug_build = project_root / 'build_debug'
    if debug_build.exists():
        print("[OK] Debug构建目录存在:", debug_build)
        checks.append(True)
    else:
        print("[WARN] Debug构建目录不存在，请运行:")
        print("      cmake -B build_debug -DCMAKE_BUILD_TYPE=Debug")
        checks.append(False)
    
    # 3. 检查CLion运行配置
    run_config = project_root / '.idea' / 'runConfigurations' / 'VulkanEngine_Debug.xml'
    if run_config.exists():
        try:
            content = run_config.read_text(encoding='utf-8')
            if 'TARGET_NAME="vulkan-engine"' in content:
                print("[OK] CLion调试配置正确")
                checks.append(True)
            else:
                print("[ERROR] CLion配置目标名称错误，应为'vulkan-engine'")
                print("      请运行 fix_clion_config.bat 修复")
                checks.append(False)
        except:
            print("[ERROR] 无法读取CLion配置")
            checks.append(False)
    else:
        print("[WARN] CLion调试配置文件不存在")
        checks.append(False)
    
    # 4. 检查可执行文件
    exe_name = "VulkanEngine.exe"
    debug_exe = project_root / 'build_debug' / 'bin' / exe_name
    if debug_exe.exists():
        print("[OK] Debug可执行文件存在:", debug_exe)
        checks.append(True)
    else:
        print("[WARN] Debug可执行文件不存在，请构建项目:")
        print("      cmake --build build_debug --config Debug")
        checks.append(False)
    
    print("\n" + "=" * 60)
    passed = sum(checks)
    total = len(checks)
    
    print(f"检查结果: {passed}/{total} 项通过")
    
    if passed == total:
        print("\n[SUCCESS] 调试环境配置完成!")
        print("\n下一步:")
        print("  1. 打开CLion")
        print("  2. 选择 'VulkanEngine (Debug)' 运行配置")
        print("  3. 在main()函数设置断点")
        print("  4. 按Shift+F9开始调试")
    else:
        print("\n[ACTION NEEDED] 请根据上方提示修复问题")
        print("\n修复建议:")
        print("  1. 运行: fix_clion_config.bat")
        print("  2. 运行: tools/setup_debug_env.bat")
        print("  3. 或手动运行调试验证脚本: python debug_validate.py")
    
    return 0 if passed == total else 1

if __name__ == "__main__":
    sys.exit(main())
@echo off
echo ============================================
echo VulkanEngine项目清理工具
echo ============================================
echo.

set PROJECT_ROOT=%~dp0..
cd /d "%PROJECT_ROOT%"

echo 正在清理构建目录...

echo 1. 清理build_conan目录...
if exist "build_conan" (
    echo   删除 build_conan
    rmdir /s /q "build_conan"
)

echo.
echo 2. 清理build_debug目录...
if exist "build_debug" (
    echo   删除 build_debug
    rmdir /s /q "build_debug"
)

echo.
echo 3. 清理build_test目录...
if exist "build_test" (
    echo   删除 build_test
    rmdir /s /q "build_test"
)

echo.
echo 4. 清理build目录...
if exist "build" (
    echo   删除 build
    rmdir /s /q "build"
)

echo.
echo 5. 清理临时文件...
del /q /f "*.pyc" 2>nul
del /q /f "*.log" 2>nul
del /q /f "*.tmp" 2>nul
del /q /f "*.cache" 2>nul

echo.
echo 6. 清理IDE临时文件（保留配置）...
if exist ".idea\workspace.xml" (
    echo   保留IDE工作空间文件
)

echo.
echo ============================================
echo 清理完成！
echo ============================================
echo.
echo 注意事项:
echo   1. 构建目录已清理，需要重新配置CMake
echo   2. 源代码和配置文件保持不变
echo   3. .gitignore已配置忽略构建目录
echo.
echo 下一步操作:
echo   1. 重新配置CMake: cmake -B build_debug -DCMAKE_BUILD_TYPE=Debug
echo   2. 构建项目: cmake --build build_debug --config Debug
echo   3. 运行: .\build_debug\bin\VulkanEngine.exe
echo.
pause
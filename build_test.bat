@echo off
echo 正在构建Vulkan项目...

REM 检查是否安装了Visual Studio或CMake兼容的编译器
if not exist build mkdir build
cd build

REM 配置项目
cmake .. -A x64
if %ERRORLEVEL% NEQ 0 (
    echo CMake配置失败
    exit /b %ERRORLEVEL%
)

REM 构建项目
cmake --build . --config Debug
if %ERRORLEVEL% NEQ 0 (
    echo 构建失败
    exit /b %ERRORLEVEL%
)

echo 构建成功！
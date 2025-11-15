@echo off
echo Building Vulkan Hello Triangle Application with vcpkg...

REM 检查vcpkg是否存在
where vcpkg >nul 2>&1
if errorlevel 1 (
  echo ERROR: vcpkg not found in PATH
  echo Please install vcpkg and add it to your PATH
  echo Or run this script from vcpkg directory
  exit /b 1
)

REM 安装依赖
echo Installing dependencies...
vcpkg install glfw3:x64-windows glm:x64-windows
if errorlevel 1 (
  echo ERROR: Failed to install dependencies with vcpkg
  exit /b 1
)

REM 创建构建目录
if not exist build_mingw mkdir build_mingw

REM 运行CMake配置（使用vcpkg工具链）
echo Configuring with CMake and vcpkg...
cmake -S . -B build_mingw -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake"
if errorlevel 1 (
  echo ERROR: CMake configuration failed
  exit /b 1
)

REM 构建项目
echo Building project...
cmake --build build_mingw --config Debug
if errorlevel 1 (
  echo ERROR: Build failed
  exit /b 1
)

echo Build completed successfully!
echo Executable location: build_mingw\bin\vulkan.exe
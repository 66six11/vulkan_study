@echo off
echo ========================================
echo Vulkan Engine - Conan 2.0 Build
echo ========================================

REM 验证Conan安装
echo.
echo [1/4] Checking Conan installation...
conan --version >nul 2>&1
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Conan not installed!
    echo Please install: pip install conan
    echo.
    exit /b 1
)
echo Conan installed successfully!

REM 检查Vulkan SDK
echo.
echo [2/4] Checking Vulkan SDK...
if not defined VULKAN_SDK (
    echo [WARNING] VULKAN_SDK not defined
    echo Make sure Vulkan SDK is installed
) else (
    echo Vulkan SDK: %VULKAN_SDK%
)

REM 安装Conan依赖
echo.
echo ========================================
echo [3/4] Installing Conan dependencies...
echo ========================================
conan install . --build=missing -s build_type=Release --output-folder=build
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Conan install failed!
    echo.
    exit /b 1
)
echo Conan dependencies installed!

REM 配置并构建
echo.
echo ========================================
echo [4/4] Configuring and building...
echo ========================================
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE=build/build/generators/conan_toolchain.cmake ^
    -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed!
    exit /b 1
)

cmake --build build --config Release --parallel
if %errorlevel% neq 0 (
    echo [ERROR] Build failed!
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo Executable: build\Release\vulkan-engine.exe
echo ========================================

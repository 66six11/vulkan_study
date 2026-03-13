@echo off
echo ========================================
echo Vulkan Engine - Conan 2.0 Build
echo ========================================

REM Switch to project root (script is in tools/ subdirectory)
cd /d "%~dp0\.."
echo Project root: %cd%
echo.

REM Verify Conan installation
echo.
echo [1/5] Checking Conan installation...
conan --version >nul 2>&1
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Conan not installed!
    echo Please install: pip install conan==2.0.16
    echo.
    pause
    exit /b 1
)
echo Conan installed successfully!

REM Check Vulkan SDK
echo.
echo [2/5] Checking Vulkan SDK...
if not defined VULKAN_SDK (
    echo [WARNING] VULKAN_SDK not defined
    echo Make sure Vulkan SDK is installed and VULKAN_SDK environment variable is set
) else (
    echo Vulkan SDK: %VULKAN_SDK%
)

REM Select build type
echo.
echo Available build types:
echo   1. Debug
echo   2. Release
echo.
set /p BUILD_CHOICE="Select build type [1]: "
if "%BUILD_CHOICE%"=="" set BUILD_CHOICE=1

if "%BUILD_CHOICE%"=="1" (
    set BUILD_TYPE=Debug
) else if "%BUILD_CHOICE%"=="2" (
    set BUILD_TYPE=Release
) else (
    echo Invalid choice, using Debug
    set BUILD_TYPE=Debug
)
echo Selected: %BUILD_TYPE%

REM Clean option
echo.
set /p CLEAN="Clean old build? (y/N): "
if /i "%CLEAN%"=="y" (
    echo.
    echo [3/5] Cleaning old build...
    if exist build rd /s /q build
    echo Build directory cleaned
)

REM Install Conan dependencies
echo.
echo ========================================
echo Installing Conan dependencies...
echo ========================================
conan install . --build=missing -s build_type=%BUILD_TYPE% --output-folder=build
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Conan install failed!
    echo.
    echo Troubleshooting:
    echo   1. Check your internet connection
    echo   2. Run: conan cache purge "*" --all
    echo   3. Run: conan profile detect
    echo.
    pause
    exit /b 1
)

echo.
echo Conan dependencies installed successfully!

REM Configure CMake
echo.
echo ========================================
echo Configuring CMake...
echo ========================================
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE=build/build/generators/conan_toolchain.cmake ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] CMake configuration failed!
    echo.
    echo Troubleshooting:
    echo   1. Check CMake version (requires 3.25+)
    echo   2. Verify Visual Studio 2022 is installed
    echo   3. Check that Vulkan SDK is properly installed
    echo.
    pause
    exit /b 1
)

echo.
echo CMake configured successfully!

REM Build project
echo.
echo ========================================
echo Building project...
echo ========================================
cmake --build build --config %BUILD_TYPE% --parallel
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Build failed!
    echo.
    echo Check the error messages above for details.
    echo.
    pause
    exit /b 1
)

echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo.
echo Build type: %BUILD_TYPE%
echo Executable: build\%BUILD_TYPE%\vulkan-engine.exe
echo.
echo Run the executable:
echo   cd build\%BUILD_TYPE%
echo   vulkan-engine.exe
echo.
pause
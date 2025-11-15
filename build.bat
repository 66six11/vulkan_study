@echo off
echo Building Vulkan Hello Triangle Application...

REM 设置环境变量（如果需要）
if defined VULKAN_SDK (
  echo Using Vulkan SDK: %VULKAN_SDK%
) else (
  echo WARNING: VULKAN_SDK not defined. Make sure Vulkan SDK is installed.
)

REM 检查是否安装了Visual Studio
if not defined VCINSTALLDIR (
  echo Checking for Visual Studio installation...
  call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
  if errorlevel 1 (
    echo Checking alternative Visual Studio installation...
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    if errorlevel 1 (
      echo ERROR: Could not find Visual Studio build tools
      exit /b 1
    )
  )
)

REM 创建构建目录
if not exist build_vs mkdir build_vs

REM 运行CMake配置
echo Configuring with CMake...
cmake -S . -B build_vs -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
  echo ERROR: CMake configuration failed
  exit /b 1
)

REM 构建项目
echo Building project...
cmake --build build_vs --config Debug
if errorlevel 1 (
  echo ERROR: Build failed
  exit /b 1
)

echo Build completed successfully!
echo Executable location: build_vs\Debug\vulkan.exe
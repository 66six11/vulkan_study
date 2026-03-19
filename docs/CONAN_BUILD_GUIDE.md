# Conan 2.0 构建指南

## 问题说明

当前项目配置了Conan 2.0作为包管理器（`conanfile.py`），但构建脚本`build.bat`没有使用Conan，导致依赖管理不一致。

## 解决方案

### 方案1: 使用Conan构建（推荐）

这是符合项目设计的正确方式，使用Conan 2.0管理所有依赖。

#### 前置要求

1. **安装Conan 2.0**

```bash
pip install conan==2.0.16
```

2. **验证安装**

```bash
conan --version
# 应该显示: Conan version 2.x.x
```

3. **创建Conan配置文件**
   首次使用需要配置Conan：

```bash
conan profile detect
```

#### 构建步骤

**Windows (批处理脚本)**

```batch
@echo off
echo Building Vulkan Engine with Conan 2.0...

REM 检查Conan是否安装
where conan >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Conan not found. Please install: pip install conan==2.0.16
    exit /b 1
)

REM 检查Vulkan SDK
if not defined VULKAN_SDK (
    echo WARNING: VULKAN_SDK not defined. Make sure Vulkan SDK is installed.
)

REM 安装Conan依赖
echo Installing Conan dependencies...
conan install . --build=missing -s build_type=Debug -pr:b=default -pr:h=default --output-folder=build
if %errorlevel% neq 0 (
    echo ERROR: Conan install failed
    exit /b 1
)

REM 配置CMake（使用Conan生成的toolchain）
echo Configuring with CMake...
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE=build/build/generators/conan_toolchain.cmake ^
    -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed
    exit /b 1
)

REM 构建项目
echo Building project...
cmake --build build --config Debug
if %errorlevel% neq 0 (
    echo ERROR: Build failed
    exit /b 1
)

echo Build completed successfully!
echo Executable location: build\Debug\vulkan-engine.exe
```

**Linux/macOS (Shell脚本)**

```bash
#!/bin/bash

echo "Building Vulkan Engine with Conan 2.0..."

# 检查Conan是否安装
if ! command -v conan &> /dev/null; then
    echo "ERROR: Conan not found. Please install: pip install conan==2.0.16"
    exit 1
fi

# 检查Vulkan SDK
if [ -z "$VULKAN_SDK" ]; then
    echo "WARNING: VULKAN_SDK not defined. Make sure Vulkan SDK is installed."
fi

# 安装Conan依赖
echo "Installing Conan dependencies..."
conan install . --build=missing -s build_type=Debug --output-folder=build
if [ $? -ne 0 ]; then
    echo "ERROR: Conan install failed"
    exit 1
fi

# 配置CMake（使用Conan生成的toolchain）
echo "Configuring with CMake..."
cmake -S . -B build -G "Unix Makefiles" \
    -DCMAKE_TOOLCHAIN_FILE=build/build/generators/conan_toolchain.cmake \
    -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
if [ $? -ne 0 ]; then
    echo "ERROR: CMake configuration failed"
    exit 1
fi

# 构建项目
echo "Building project..."
cmake --build build --config Debug
if [ $? -ne 0 ]; then
    echo "ERROR: Build failed"
    exit 1
fi

echo "Build completed successfully!"
```

#### 使用CMake Presets（推荐）

Conan 2.0支持CMake Presets，使用更方便：

```bash
# 安装Conan依赖
conan install . --build=missing -s build_type=Debug --output-folder=build

# 使用CMake Presets配置
cmake --preset conan-debug

# 构建
cmake --build --preset conan-debug
```

### 方案2: 使用vcpkg构建（备选）

如果团队已经配置了vcpkg，可以切换到vcpkg。

#### 前置要求

1. **安装vcpkg**

```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat  # Windows
# 或
./bootstrap-vcpkg.sh  # Linux/macOS
```

2. **集成到CMake**
   设置环境变量或在CMake中指定：

```bash
set VCPKG_ROOT=C:\path\to\vcpkg
set VCPKG_DEFAULT_TRIPLET=x64-windows
```

#### vcpkg.json配置

创建`vcpkg.json`文件：

```json
{
  "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg.schema.json",
  "name": "vulkan-engine",
  "version": "2.0.0",
  "description": "Modern C++ Vulkan rendering engine",
  "dependencies": [
    "glfw3",
    "glm",
    "stb",
    {
      "name": "vulkan",
      "platform": "!windows"
    }
  ],
  "features": {
    "tests": {
      "description": "Build tests",
      "dependencies": [
        "catch2",
        "benchmark"
      ]
    }
  }
}
```

#### vcpkg构建脚本

```batch
@echo off
echo Building Vulkan Engine with vcpkg...

REM 检查vcpkg
if not defined VCPKG_ROOT (
    echo ERROR: VCPKG_ROOT not defined
    exit /b 1
)

REM 安装vcpkg依赖
echo Installing vcpkg dependencies...
%VCPKG_ROOT%\vcpkg install glfw3:x64-windows glm:x64-windows stb:x64-windows
if %errorlevel% neq 0 (
    echo ERROR: vcpkg install failed
    exit /b 1
)

REM 配置CMake（使用vcpkg toolchain）
echo Configuring with CMake...
cmake -S . -B build_vcpkg -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET=x64-windows
if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed
    exit /b 1
)

REM 构建项目
echo Building project...
cmake --build build_vcpkg --config Debug
if %errorlevel% neq 0 (
    echo ERROR: Build failed
    exit /b 1
)

echo Build completed successfully!
```

### 方案3: 混合模式（不推荐）

同时支持Conan和vcpkg，但会增加维护复杂度。

## 推荐方案对比

| 特性      | Conan 2.0 | vcpkg |
|---------|-----------|-------|
| 配置复杂度   | 中等        | 简单    |
| 依赖管理    | 声明式       | 声明式   |
| 跨平台支持   | 优秀        | 良好    |
| CMake集成 | 原生支持      | 原生支持  |
| 二进制缓存   | 内置        | 支持    |
| 自定义配置   | 灵活        | 中等    |
| 社区支持    | 活跃        | 活跃    |
| 学习曲线    | 中等        | 简单    |

## 当前问题修复

### 问题分析

1. **CMakeLists.txt中的vcpkg引用**（第39-80行）
    - 包含了vcpkg路径检查
    - 与Conan配置冲突

2. **build.bat未使用Conan**
    - 直接调用CMake
    - 没有安装Conan依赖

3. **CMakeUserPresets.json**
    - 已配置为Conan模式
    - 但未被build.bat使用

### 修复步骤

#### 1. 清理CMakeLists.txt中的vcpkg引用

删除或注释掉第38-80行的vcpkg相关代码：

```cmake
# 删除这部分代码
if (DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif ()

# 删除GLFW手动搜索代码
find_library(GLFW_LIBRARY glfw3 glfw PATHS ...)
find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h PATHS ...)

# 删除GLM手动搜索代码
find_path(GLM_INCLUDE_DIR glm/glm.hpp PATHS ...)
```

简化为：

```cmake
# Find dependencies (expected to be provided by Conan)
find_package(Vulkan REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
```

#### 2. 更新build.bat使用Conan

```batch
@echo off
echo Building Vulkan Engine with Conan 2.0...

REM 检查Conan
where conan >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: Conan not found. Install: pip install conan==2.0.16
    exit /b 1
)

REM 安装Conan依赖
conan install . --build=missing -s build_type=Debug --output-folder=build
if %errorlevel% neq 0 (
    echo ERROR: Conan install failed
    exit /b 1
)

REM 配置CMake
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE=build/build/generators/conan_toolchain.cmake
if %errorlevel% neq 0 (
    echo ERROR: CMake configuration failed
    exit /b 1
)

REM 构建
cmake --build build --config Debug
if %errorlevel% neq 0 (
    echo ERROR: Build failed
    exit /b 1
)

echo Build completed successfully!
```

#### 3. 添加conan_build.bat（新脚本）

```batch
@echo off
echo ========================================
echo Vulkan Engine - Conan 2.0 Build
echo ========================================

REM 验证Conan安装
echo Checking Conan installation...
conan --version
if %errorlevel% neq 0 (
    echo.
    echo [ERROR] Conan not installed!
    echo Please install: pip install conan==2.0.16
    pause
    exit /b 1
)

REM 检查Vulkan SDK
echo.
echo Checking Vulkan SDK...
if not defined VULKAN_SDK (
    echo [WARNING] VULKAN_SDK not defined
    echo Make sure Vulkan SDK is installed
)

REM 选择构建类型
set BUILD_TYPE=Debug
set /p BUILD_TYPE="Enter build type (Debug/Release) [%BUILD_TYPE%]: "

REM 清理旧构建
echo.
echo Cleaning old build...
if exist build rd /s /q build

REM 安装Conan依赖
echo.
echo ========================================
echo Installing Conan dependencies...
echo ========================================
conan install . --build=missing -s build_type=%BUILD_TYPE% --output-folder=build
if %errorlevel% neq 0 (
    echo [ERROR] Conan install failed!
    pause
    exit /b 1
)

REM 配置CMake
echo.
echo ========================================
echo Configuring CMake...
echo ========================================
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE=build/build/generators/conan_toolchain.cmake ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed!
    pause
    exit /b 1
)

REM 构建项目
echo.
echo ========================================
echo Building project...
echo ========================================
cmake --build build --config %BUILD_TYPE% --parallel
if %errorlevel% neq 0 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)

REM 完成
echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo Executable: build\%BUILD_TYPE%\vulkan-engine.exe
echo ========================================
pause
```

## 验证修复

### 测试步骤

1. **清理旧构建**

```bash
# Windows
rd /s /q build
rd /s /q build_vs
rd /s /q build_test

# Linux/macOS
rm -rf build build_vs build_test
```

2. **使用Conan构建**

```bash
# Windows
conan_build.bat

# Linux/macOS
chmod +x conan_build.sh
./conan_build.sh
```

3. **验证依赖**

```bash
# 检查Conan缓存
conan cache list

# 验证GLFW
conan cache list glfw

# 验证GLM
conan cache list glm
```

4. **运行测试**

```bash
# Windows
build\Debug\vulkan-engine.exe

# Linux/macOS
./build/vulkan-engine
```

## 迁移指南

### 从vcpkg迁移到Conan

1. **备份当前配置**

```bash
git add .
git commit -m "Backup before Conan migration"
```

2. **移除vcpkg配置**

- 删除vcpkg.json（如果存在）
- 清理CMakeLists.txt中的vcpkg引用

3. **更新构建脚本**

- 使用新的conan_build.bat
- 删除旧的build.bat

4. **测试构建**

```bash
conan install . --build=missing -s build_type=Debug
cmake --preset conan-debug
cmake --build --preset conan-debug
```

5. **提交更改**

```bash
git add .
git commit -m "Migrate to Conan 2.0"
```

## 常见问题

### Q: Conan安装依赖失败？

**A**: 尝试以下步骤：

```bash
# 清理Conan缓存
conan cache purge "*" --all

# 重新安装
conan install . --build=missing
```

### Q: CMake找不到Conan提供的库？

**A**: 确保正确设置toolchain文件：

```bash
-DCMAKE_TOOLCHAIN_FILE=build/build/generators/conan_toolchain.cmake
```

### Q: 想同时使用Conan和系统库？

**A**: 在conanfile.py中配置：

```python
def requirements(self):
    # 只在系统找不到时使用Conan
    if not self.options.use_system_deps:
        self.requires("glfw/3.3.8")
```

### Q: 如何添加新依赖？

**A**: 编辑conanfile.py：

```python
def requirements(self):
    self.requires("new-package/1.0.0")
```

然后重新运行：

```bash
conan install . --build=missing
```

## 总结

**推荐使用Conan 2.0**，因为：

- 项目已经配置了Conan
- 提供更好的依赖管理
- 支持二进制缓存加速构建
- 跨平台支持更一致

修复步骤：

1. 清理CMakeLists.txt中的vcpkg引用
2. 使用新的conan_build.bat脚本
3. 删除旧的build.bat或重命名为build_legacy.bat
4. 更新文档说明使用Conan

---

**版本**: 1.0
**更新日期**: 2026年3月13日

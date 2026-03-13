#!/bin/bash

echo "========================================"
echo "Vulkan Engine - Conan 2.0 Build"
echo "========================================"

# 验证Conan安装
echo ""
echo "[1/5] Checking Conan installation..."
if ! command -v conan &> /dev/null; then
    echo ""
    echo "[ERROR] Conan not installed!"
    echo "Please install: pip install conan==2.0.16"
    echo ""
    exit 1
fi

echo "Conan installed successfully!"
conan --version

# 检查Vulkan SDK
echo ""
echo "[2/5] Checking Vulkan SDK..."
if [ -z "$VULKAN_SDK" ]; then
    echo "[WARNING] VULKAN_SDK not defined"
    echo "Make sure Vulkan SDK is installed and VULKAN_SDK environment variable is set"
else
    echo "Vulkan SDK: $VULKAN_SDK"
fi

# 选择构建类型
echo ""
echo "Available build types:"
echo "  1. Debug"
echo "  2. Release"
echo ""
read -p "Select build type [1]: " BUILD_CHOICE
BUILD_CHOICE=${BUILD_CHOICE:-1}

if [ "$BUILD_CHOICE" = "1" ]; then
    BUILD_TYPE="Debug"
elif [ "$BUILD_CHOICE" = "2" ]; then
    BUILD_TYPE="Release"
else
    echo "Invalid choice, using Debug"
    BUILD_TYPE="Debug"
fi

echo "Selected: $BUILD_TYPE"

# 清理选项
echo ""
read -p "Clean old build? (y/N): " CLEAN
if [[ "$CLEAN" =~ ^[Yy]$ ]]; then
    echo ""
    echo "[3/5] Cleaning old build..."
    rm -rf build
    echo "Build directory cleaned"
fi

# 安装Conan依赖
echo ""
echo "========================================"
echo "Installing Conan dependencies..."
echo "========================================"
conan install . --build=missing -s build_type=$BUILD_TYPE --output-folder=build
if [ $? -ne 0 ]; then
    echo ""
    echo "[ERROR] Conan install failed!"
    echo ""
    echo "Troubleshooting:"
    echo "  1. Check your internet connection"
    echo "  2. Run: conan cache purge \"*\" --all"
    echo "  3. Run: conan profile detect"
    echo ""
    exit 1
fi

echo ""
echo "Conan dependencies installed successfully!"

# 配置CMake
echo ""
echo "========================================"
echo "Configuring CMake..."
echo "========================================"
cmake -S . -B build -G "Unix Makefiles" \
    -DCMAKE_TOOLCHAIN_FILE=build/build/generators/conan_toolchain.cmake \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_POLICY_DEFAULT_CMP0091=NEW
if [ $? -ne 0 ]; then
    echo ""
    echo "[ERROR] CMake configuration failed!"
    echo ""
    echo "Troubleshooting:"
    echo "  1. Check CMake version (requires 3.25+)"
    echo "  2. Verify build tools are installed"
    echo "  3. Check that Vulkan SDK is properly installed"
    echo ""
    exit 1
fi

echo ""
echo "CMake configured successfully!"

# 构建项目
echo ""
echo "========================================"
echo "Building project..."
echo "========================================"
cmake --build build --config $BUILD_TYPE --parallel $(nproc)
if [ $? -ne 0 ]; then
    echo ""
    echo "[ERROR] Build failed!"
    echo ""
    echo "Check error messages above for details."
    echo ""
    exit 1
fi

echo ""
echo "========================================"
echo "Build completed successfully!"
echo "========================================"
echo ""
echo "Build type: $BUILD_TYPE"
echo "Executable: build/vulkan-engine"
echo ""
echo "Run executable:"
echo "  cd build"
echo "  ./vulkan-engine"
echo ""

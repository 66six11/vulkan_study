# Vulkan Engine - Agent 上下文文档

## 项目概述

**Vulkan Engine** 是一个现代化的 C++20 Vulkan 渲染引擎，采用声明式 Render Graph 架构，专注于类型安全、高性能和可维护性。

- **版本**: 2.0.0
- **语言**: C++20
- **图形 API**: Vulkan 1.3+
- **构建系统**: CMake 3.25+ + Conan 2.0
- **许可证**: MIT

## 架构设计

### 分层架构

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
│              (应用逻辑、游戏循环、事件处理)                    │
├─────────────────────────────────────────────────────────────┤
│                   Rendering Layer                           │
│         (Render Graph、资源管理、场景管理)                    │
├─────────────────────────────────────────────────────────────┤
│                   Vulkan Backend                            │
│      (设备管理、管线、资源、同步、命令缓冲管理)                 │
├─────────────────────────────────────────────────────────────┤
│                   Platform Layer                            │
│           (窗口管理、输入系统、文件系统)                       │
├─────────────────────────────────────────────────────────────┤
│                     Core Layer                              │
│            (数学库、内存管理、工具函数、日志)                   │
└─────────────────────────────────────────────────────────────┘
```

### 核心设计原则

1. **RAII 资源管理**: 所有 Vulkan 资源通过强类型包装器自动管理生命周期
2. **类型安全**: 使用 Tag 类型系统防止 Vulkan 对象误用
3. **声明式渲染**: Render Graph 描述渲染流程，自动处理资源和同步
4. **现代 C++**: 充分利用 C++20 Concepts、Coroutines、Modules

## 项目结构

```
vulkan-engine/
├── CMakeLists.txt           # 主 CMake 配置
├── conanfile.py             # Conan 包管理配置
├── .clang-format            # 代码格式化配置
│
├── include/                 # 公共头文件
│   ├── application/         # 应用层接口
│   │   ├── app/Application.hpp
│   │   └── config/Config.hpp
│   ├── core/                # 核心系统接口
│   │   ├── math/Vector.hpp
│   │   ├── memory/
│   │   └── utils/Logger.hpp
│   ├── platform/            # 平台抽象接口
│   │   ├── windowing/Window.hpp
│   │   ├── input/InputManager.hpp
│   │   └── filesystem/FileSystem.hpp
│   ├── rendering/           # 渲染系统接口
│   │   ├── render_graph/RenderGraph.hpp
│   │   ├── resources/ResourceManager.hpp
│   │   ├── scene/Scene.hpp
│   │   └── shaders/ShaderManager.hpp
│   └── vulkan/              # Vulkan 后端接口
│       ├── device/Device.hpp
│       ├── resources/Buffer.hpp, Image.hpp
│       ├── pipelines/Pipeline.hpp
│       └── sync/Synchronization.hpp
│
├── src/                     # 源代码实现
│   ├── main.cpp             # 应用入口
│   ├── application/         # 应用层实现
│   ├── core/                # 核心系统实现
│   ├── platform/            # 平台抽象实现
│   ├── rendering/           # 渲染系统实现
│   └── vulkan/              # Vulkan 后端实现
│
├── shaders/                 # GLSL 着色器
│   ├── shader.vert
│   └── shader.frag
│
├── CMake/                   # CMake 模块
│   ├── Options.cmake        # 构建选项
│   ├── CompilerFlags.cmake  # 编译器标志
│   ├── Dependencies.cmake   # 依赖配置
│   └── Modules/             # 子模块配置
│
├── tools/                   # 开发工具
│   ├── build.bat            # Windows 构建脚本
│   ├── conan_build.bat
│   └── clean_project.bat
│
├── docs/                    # 文档
│   ├── AGENT_GUIDE.md       # Agent 使用指南
│   ├── QUICK_REFERENCE.md   # 快速参考
│   └── CONAN_BUILD_GUIDE.md # 构建指南
│
└── .github/agents/          # Agent 定义
    └── Vulkan Graphics API expert.agent.md
```

## 构建系统

### 依赖管理 (Conan 2.0)

```python
# conanfile.py 中的依赖
self.requires("glfw/3.3.8")       # 窗口管理
self.requires("glm/0.9.9.8")      # 数学库
self.requires("stb/cci.20230920") # 图像处理
self.requires("taskflow/3.6.0")   # 异步任务 (可选)
```

### 构建命令

```bash
# 完整构建 (Windows)
conan install . --build=missing -s build_type=Release
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_TOOLCHAIN_FILE=build/build/generators/conan_toolchain.cmake
cmake --build build --config Release

# 或使用脚本
tools\build.bat

# 清理构建
tools\clean_project.bat
```

### CMake 选项

| 选项                                | 默认值 | 说明              |
|-----------------------------------|-----|-----------------|
| `VULKAN_ENGINE_USE_RENDER_GRAPH`  | ON  | 启用 Render Graph |
| `VULKAN_ENGINE_USE_ASYNC_LOADING` | ON  | 启用异步资源加载        |
| `VULKAN_ENGINE_USE_HOT_RELOAD`    | ON  | 启用着色器热重载        |
| `VULKAN_ENGINE_ENABLE_VALIDATION` | ON  | 启用验证层           |
| `VULKAN_ENGINE_BUILD_TESTS`       | OFF | 构建测试            |
| `VULKAN_ENGINE_ENABLE_LTO`        | OFF | 启用链接时优化         |

## 编码规范

### 命名约定

```cpp
// 命名空间: 小写 + 下划线
namespace vulkan_engine::vulkan { }

// 类名: PascalCase
class DeviceManager { };
class RenderGraph { };

// 函数: camelCase
void initialize();
bool isKeyPressed(Key key);

// 成员变量: camelCase + 下划线后缀
class Example {
    uint32_t bufferSize_;
    std::string name_;
};

// 常量: UPPER_SNAKE_CASE
constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

// 枚举: PascalCase + 描述性名称
enum class KeyAction {
    Press,
    Release,
    Repeat
};
```

### C++20 关键特性

```cpp
// Concepts 约束
template<typename T>
concept VulkanDevice = requires(T device) {
    { device.instance() } -> std::same_as<Instance>;
    { device.graphics_queue() } -> std::same_as<Queue>;
};

// 强类型 Vulkan 句柄包装器
template<typename Tag, typename HandleType>
class VulkanHandleBase {
    HandleType handle_;
public:
    constexpr bool valid() const noexcept { 
        return handle_ != VK_NULL_HANDLE; 
    }
};

using Device = VulkanHandleBase<DeviceTag, VkDevice>;

// RAII 资源管理
class Buffer {
    VkBuffer buffer_;
    VmaAllocation allocation_;
public:
    ~Buffer() { 
        vmaDestroyBuffer(allocator_, buffer_, allocation_); 
    }
    Buffer(const Buffer&) = delete;
    Buffer(Buffer&& other) noexcept = default;
};
```

## Vulkan 最佳实践

### 资源管理

```cpp
// 使用强类型包装器
Device device = device_manager.device();
if (device.valid()) { /* ... */ }

// RAII 自动管理生命周期
class Texture {
    Image image_;
    ImageView view_;
    VmaAllocation allocation_;
    // 析构时自动释放
};
```

### 同步管理

```cpp
// GPU-GPU 同步: Semaphore
void submitWithSemaphore(Queue queue, CommandBuffer cmd, 
                         Semaphore wait, Semaphore signal);

// CPU-GPU 同步: Fence
void waitForFence(Fence fence, uint64_t timeout);

// 执行屏障: Pipeline Barriers
void transitionImageLayout(Image image, VkImageLayout old, VkImageLayout new);
```

### 性能优化

```cpp
// 最小化 draw calls
void batchDrawCalls(const std::vector<Mesh>& meshes);

// 命令缓冲复用
CommandBuffer cb = command_pool.allocate();
cb.begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
// 录制命令...
cb.end();

// 多线程录制
std::vector<std::future<CommandBuffer>> futures;
for (auto& thread_data : thread_datas) {
    futures.push_back(std::async(std::launch::async, 
        [&]() { return recordCommands(thread_data); }));
}
```

## 调试与工具

### Validation Layers

```cpp
// 启用验证层
DeviceManager::CreateInfo create_info{
    .enable_validation = true,
    .enable_debug_utils = true
};

// 自定义回调
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* user_data) {
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        logger::warn(data->pMessage);
    }
    return VK_FALSE;
}
```

### 推荐工具

| 工具                   | 用途           |
|----------------------|--------------|
| **RenderDoc**        | GPU 调试、帧捕获   |
| **Tracy**            | CPU/GPU 性能分析 |
| **NSight**           | NVIDIA 性能工具  |
| **Clang-Tidy**       | 静态分析         |
| **AddressSanitizer** | 内存错误检测       |

## 常见任务

### 添加新模块

1. 在 `include/` 创建头文件
2. 在 `src/` 创建实现文件
3. 在 `CMake/Modules/` 添加 CMake 配置
4. 更新主 CMakeLists.txt

### 添加新依赖

1. 更新 `conanfile.py` 中的 `requirements()`
2. 运行 `conan install . --build=missing`
3. 在 CMake 中使用 `find_package()`

### 实现 Render Graph Pass

```cpp
class MyRenderPass : public RenderPass {
public:
    void setup(RenderGraphBuilder& builder) override {
        builder.readTexture("input_tex");
        builder.writeTexture("output_tex");
    }
    
    void execute(CommandBuffer& cmd, const RenderContext& ctx) override {
        // 绑定管线、描述符
        // 绘制命令
    }
};
```

## 已知问题与解决方案

### 1. MSVC 运行时库冲突

**问题**: 链接 GLFW 时出现 CRT 符号未解析

**解决**: 已在 CMakeLists.txt 中设置强制使用动态运行时库

```cmake
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
```

### 2. InputManager 数组越界

**问题**: Debug 模式下数组下标越界断言失败

**解决**: 使用映射表限制查询范围，避免遍历所有 GLFW 键值

```cpp
static constexpr std::array<int, KeyCount> key_map = { ... };
for (size_t i = 0; i < KeyCount; ++i) { ... }
```

## 参考文档

### 项目文档

- [新架构规划](新架构规划.md)
- [现代化重构架构方案](现代化重构架构方案.md)
- [Agent 使用指南](docs/AGENT_GUIDE.md)
- [快速参考](docs/QUICK_REFERENCE.md)

### 外部资源

- [Vulkan 规范](https://registry.khronos.org/vulkan/)
- [Vulkan Guide](https://github.com/KhronosGroup/Vulkan-Guide)
- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [LunarG SDK](https://www.lunarg.com/vulkan-sdk/)

---

**最后更新**: 2026-03-13
**文档版本**: 1.0.0

# Vulkan Engine - Agent 上下文文档

## 项目概述

**Vulkan Engine** 是一个现代化的 C++20 Vulkan 渲染引擎，采用声明式 Render Graph 架构，专注于类型安全、高性能和可维护性。

- **版本**: 2.0.0
- **分支**: 2.2
- **语言**: C++20
- **图形 API**: Vulkan 1.3+
- **构建系统**: CMake 3.25+ + Conan 2.0
- **许可证**: MIT
- **当前阶段**: Phase 3 - 编辑器集成与 Viewport 系统

## 架构设计

### 分层架构

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
│              (应用逻辑、游戏循环、事件处理)                    │
├─────────────────────────────────────────────────────────────┤
│                   Rendering Layer                           │
│         (Render Graph、资源管理、场景管理、材质系统)           │
├─────────────────────────────────────────────────────────────┤
│                   Vulkan Backend                            │
│      (设备管理、管线、资源、同步、命令缓冲管理)                 │
├─────────────────────────────────────────────────────────────┤
│                   Platform Layer                            │
│           (窗口管理、输入系统、文件系统)                       │
├─────────────────────────────────────────────────────────────┤
│                     Core Layer                              │
│            (数学库、内存管理、工具函数、日志)                   │
├─────────────────────────────────────────────────────────────┤
│                   Editor Layer                              │
│              (ImGui 编辑器、视口面板、调试工具)                │
└─────────────────────────────────────────────────────────────┘
```

### 核心设计原则

1. **RAII 资源管理**: 所有 Vulkan 资源通过强类型包装器自动管理生命周期
2. **类型安全**: 使用 Tag 类型系统防止 Vulkan 对象误用
3. **声明式渲染**: Render Graph 描述渲染流程，自动处理资源和同步
4. **现代 C++**: 充分利用 C++20 Concepts、Coroutines、Modules
5. **职责分离**: RenderTarget 只管附件，Viewport 只管视窗逻辑，RenderPass 只管渲染流程

## 项目结构

```
vulkan-engine/
├── CMakeLists.txt           # 主 CMake 配置
├── conanfile.py             # Conan 包管理配置
├── .clang-format            # 代码格式化配置
│
├── include/                 # 公共头文件 (42 个 .hpp)
│   ├── application/         # 应用层接口
│   │   ├── app/Application.hpp
│   │   └── config/Config.hpp
│   ├── core/                # 核心系统接口
│   │   ├── math/Vector.hpp, Camera.hpp
│   │   └── utils/Logger.hpp
│   ├── editor/              # 编辑器层接口
│   │   ├── Editor.hpp
│   │   └── ImGuiManager.hpp
│   ├── platform/            # 平台抽象接口
│   │   ├── windowing/Window.hpp
│   │   ├── input/InputManager.hpp
│   │   └── filesystem/FileSystem.hpp
│   ├── rendering/           # 渲染系统接口
│   │   ├── render_graph/    # Render Graph 核心
│   │   │   ├── RenderGraph.hpp
│   │   │   ├── RenderGraphPass.hpp
│   │   │   ├── RenderGraphResource.hpp
│   │   │   ├── CubeRenderPass.hpp
│   │   │   └── ViewportRenderPass.hpp
│   │   ├── camera/          # 相机控制
│   │   │   └── CameraController.hpp
│   │   ├── material/        # 材质系统
│   │   │   ├── Material.hpp
│   │   │   └── MaterialLoader.hpp
│   │   ├── resources/       # 资源管理
│   │   │   ├── ResourceManager.hpp
│   │   │   ├── RenderTarget.hpp
│   │   │   ├── Mesh.hpp
│   │   │   ├── ObjLoader.hpp
│   │   │   └── TextureLoader.hpp
│   │   ├── scene/Scene.hpp
│   │   ├── shaders/ShaderManager.hpp
│   │   ├── Viewport.hpp
│   │   └── SceneViewport.hpp
│   └── vulkan/              # Vulkan 后端接口
│       ├── device/Device.hpp, SwapChain.hpp
│       ├── resources/Buffer.hpp, Image.hpp
│       │   ├── DepthBuffer.hpp, UniformBuffer.hpp
│       │   └── Framebuffer.hpp
│       ├── pipelines/Pipeline.hpp, ShaderModule.hpp
│       │   └── RenderPassManager.hpp      # NEW
│       ├── command/CommandBuffer.hpp
│       ├── sync/Synchronization.hpp
│       └── utils/VulkanError.hpp
│           └── CoordinateTransform.hpp    # NEW
│
├── src/                     # 源代码实现 (37 个 .cpp)
│   ├── main.cpp             # 应用入口
│   ├── application/
│   ├── core/
│   ├── editor/
│   ├── platform/
│   ├── rendering/
│   └── vulkan/
│
├── shaders/                 # Slang 着色器
│   ├── triangle.slang       # 基础着色器
│   ├── pbr.slang            # PBR 着色器
│   ├── normal_vis.slang     # 法线可视化
│   └── *.spv                # 编译后的 SPIR-V
│
├── materials/               # 材质定义 (JSON)
│   ├── plastic.json
│   ├── metal.json
│   ├── emissive.json
│   ├── textured.json
│   └── normal_vis.json
│
├── CMake/                   # CMake 模块
│   ├── Options.cmake
│   ├── CompilerFlags.cmake
│   ├── Dependencies.cmake
│   └── Modules/             # 子模块配置
│       ├── Core.cmake
│       ├── Platform.cmake
│       ├── Rendering.cmake
│       ├── Vulkan.cmake
│       ├── Editor.cmake
│       └── Application.cmake
│
├── tools/                   # 开发工具
│   ├── build.bat
│   ├── conan_build.bat
│   └── clean_project.bat
│
├── docs/                    # 文档
│   ├── AGENT_GUIDE.md       # Agent 使用指南
│   ├── QUICK_REFERENCE.md   # 快速参考
│   ├── CONAN_BUILD_GUIDE.md # 构建指南
│   ├── ROADMAP.md           # 修复与演进路线图
│   ├── PHASE3_MIGRATION_FIXES.md  # Phase 3 修复日志
│   ├── ARCHITECTURE_REVIEW.md
│   ├── CODE_REVIEW_REPORT.md
│   ├── RENDERING_ARCHITECTURE_REVIEW.md
│   ├── REFACTORING_PLAN.md  # 架构重构方案
│   ├── code_reviews/        # 43 份代码审查报告
│   │   └── INDEX.md         # 审查索引
│   └── reviews_and_fixes/   # 修复文档
│
├── .github/agents/          # Agent 定义
│   └── Vulkan Graphics API expert.agent.md
│
└── .iflow/agents/           # iFlow Agent 配置
    ├── cpp_expert_agent.md
    ├── graphics_vulkan_agent.md
    ├── Dear ImGui Expert.agent.md
    └── ...
```

## 构建系统

### 依赖管理 (Conan 2.0)

```python
# conanfile.py 中的依赖
self.requires("glfw/3.3.8")           # 窗口管理
self.requires("glm/0.9.9.8")          # 数学库
self.requires("stb/cci.20230920")     # 图像处理
self.requires("nlohmann_json/3.11.2") # JSON 解析
self.requires("imgui/1.91.0")         # GUI 框架
self.requires("taskflow/3.6.0")       # 异步任务 (可选)
self.requires("spirv-tools/1.3.268.0")# Shader 反射 (可选)
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

# 调试构建
conan install . --build=missing -s build_type=Debug
cmake --build build --config Debug
```

### CMake 选项

| 选项                                | 默认值 | 说明              |
|-----------------------------------|-----|-----------------|
| `VULKAN_ENGINE_USE_RENDER_GRAPH`  | ON  | 启用 Render Graph |
| `VULKAN_ENGINE_USE_ASYNC_LOADING` | ON  | 启用异步资源加载        |
| `VULKAN_ENGINE_USE_HOT_RELOAD`    | ON  | 启用着色器热重载        |
| `VULKAN_ENGINE_ENABLE_VALIDATION` | ON  | 启用验证层           |
| `VULKAN_ENGINE_BUILD_TESTS`       | OFF | 构建测试            |
| `VULKAN_ENGINE_BUILD_EXAMPLES`    | ON  | 构建示例            |
| `VULKAN_ENGINE_ENABLE_LTO`        | OFF | 启用链接时优化         |

## 编码规范

### 命名约定

```cpp
// 命名空间: 小写 + 下划线
namespace vulkan_engine::vulkan { }

// 类名: PascalCase
class DeviceManager { };
class RenderGraph { };
class RenderPassManager { };  // NEW

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

## 新增组件 (Phase 2-3)

### RenderPassManager

集中管理 RenderPass 创建和缓存：

```cpp
#include "vulkan/pipelines/RenderPassManager.hpp"

auto render_pass_mgr = std::make_shared<RenderPassManager>(device);

// 获取 Present RenderPass（带深度）
VkRenderPass rp = render_pass_mgr->get_present_render_pass_with_depth(
    swap_chain_->format(),
    depth_buffer_->format()
);
```

### CoordinateTransform

OpenGL/Vulkan 坐标系转换工具：

```cpp
#include "vulkan/utils/CoordinateTransform.hpp"

// 在 Vulkan 后端进行坐标系转换
glm::mat4 proj = camera_.get_projection_matrix(fov, aspect, near, far);
glm::mat4 vulkan_proj = CoordinateTransform::opengl_to_vulkan_projection(proj);
```

### CameraController

解耦输入处理与相机逻辑：

```cpp
#include "rendering/camera/CameraController.hpp"

auto controller = std::make_unique<OrbitCameraController>();
controller->attach_camera(camera_);
controller->attach_input_manager(input_manager_);
controller->attach_viewport(viewport_);

// 在 update 中调用
controller->update(delta_time);
```

### RenderTarget & Viewport

分离渲染目标与视窗逻辑：

```cpp
// 创建 RenderTarget
auto render_target = std::make_shared<RenderTarget>();
render_target->initialize(device, {1280, 720, ...});

// 创建 Viewport
auto viewport = std::make_shared<Viewport>();
viewport->initialize(device, render_target);

// 获取 ImGui 纹理 ID
ImTextureID tex_id = viewport->imgui_texture_id();
```

### ViewportRenderPass

视窗离屏渲染通道：

```cpp
#include "rendering/render_graph/ViewportRenderPass.hpp"

ViewportRenderPass::Config config{
    .name = "SceneViewport",
    .render_target = render_target,
    .clear_color = true,
    .clear_depth = true
};
auto vp_pass = std::make_unique<ViewportRenderPass>(config);
vp_pass->add_sub_pass(std::make_unique<CubeRenderPass>(...));

render_graph_.builder().add_node(std::move(vp_pass));
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

// 使用 RenderPassManager 避免重复创建
auto rp = render_pass_mgr->get_present_render_pass_with_depth(color_fmt, depth_fmt);
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

// Timeline Semaphore (Phase 3)
TimelineSemaphore semaphore(device, initial_value);
semaphore.signal(new_value);
semaphore.wait(expected_value, timeout);
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

// 避免每帧 vkQueueWaitIdle (参见 Phase D 修复)
// 使用 FrameSyncManager 的 fence 机制
auto& sync = frame_sync_.frame(current_frame_);
vkWaitForFences(device_, 1, &sync.in_flight_fence, VK_TRUE, UINT64_MAX);
vkResetFences(device_, 1, &sync.in_flight_fence);
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

### 日志级别控制

```cpp
#include "core/utils/Logger.hpp"

// 设置日志级别
logger::set_level(logger::Level::Warning);  // 过滤 Info 级别

// 使用宏避免热路径开销
LOG_DEBUG("Debug info");   // 编译期可消除
LOG_INFO("Info message");
LOG_WARNING("Warning!");
LOG_ERROR("Error!");
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
3. CMake 使用 `file(GLOB_RECURSE)` 自动发现，无需手动修改

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

### 处理 Viewport Resize

```cpp
// 设置 resize 回调
editor_->set_viewport_resize_callback([this](uint32_t width, uint32_t height) {
    create_render_target_framebuffer();
});

// 在 Editor::begin_frame() 中检测并触发
if (viewport_->is_resize_pending()) {
    VkExtent2D new_extent = viewport_->pending_extent();
    viewport_->apply_pending_resize();  // 先重建 Image/ImageView
    if (viewport_resize_callback_) {
        viewport_resize_callback_(new_extent.width, new_extent.height);
    }
}
```

## 已知问题与解决方案

### 1. MSVC 运行时库冲突

**问题**: 链接 GLFW 时出现 CRT 符号未解析

**解决**: 已在 CMakeLists.txt 中设置强制使用动态运行时库

```cmake
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
```

### 2. 硬编码绝对路径

**问题**: 多处使用 `D:/TechArt/Vulkan/` 绝对路径，跨机器无法运行

**解决**: 正在迁移到相对路径，参见 Phase C-2 修复计划

```cpp
// 使用项目根目录相对路径
std::filesystem::path asset_root();  // 通过 argv[0] 或环境变量推导
```

### 3. InputManager 数组越界

**问题**: Debug 模式下数组下标越界断言失败

**解决**: 使用映射表限制查询范围，避免遍历所有 GLFW 键值

```cpp
static constexpr std::array<int, KeyCount> key_map = { ... };
for (size_t i = 0; i < KeyCount; ++i) { ... }
```

### 4. 每帧 vkQueueWaitIdle 阻塞

**问题**: 主循环中每帧调用 `vkQueueWaitIdle()` 破坏 CPU/GPU 并行

**解决**: 使用 FrameSyncManager 的 fence 机制，等待上上帧完成

```cpp
// 错误
vkQueueWaitIdle(graphics_queue_);  // 每帧阻塞!

// 正确
auto& sync = frame_sync_.frame(current_frame_);
vkWaitForFences(device_, 1, &sync.in_flight_fence, VK_TRUE, UINT64_MAX);
```

## 项目阶段状态

### Phase 1 ✅ 完成

- 新目录结构、CMake 模块架构
- 基础类型系统、资源描述

### Phase 2 ✅ 完成

- Render Graph 框架、资源描述类型、Pass 基类
- RenderPassManager、CameraController、CoordinateTransform
- RenderTarget/Viewport 职责分离

### Phase 3 🚧 进行中

- 编辑器集成、Viewport resize 回调
- Phase 3 修复日志参见 `docs/PHASE3_MIGRATION_FIXES.md`

### Phase 4 📋 未开始

- Render Graph 真正执行、资源系统完整实现
- ResourceManager 接入真实加载器

### Phase 5 📋 未开始

- 高级特性（异步加载、热重载、多线程）

## 代码审查状态

基于 43 份代码审查报告（覆盖 91 个文件）：

| 等级     | 数量 | 说明           |
|--------|:--:|--------------|
| 🔴 高风险 | 62 | 崩溃、链接错误、渲染错误 |
| 🟡 中风险 | 78 | 性能问题、API 误用  |
| 🟢 低风险 | 73 | 代码风格、可维护性    |

详细报告参见 `docs/code_reviews/INDEX.md`

修复路线图参见 `docs/ROADMAP.md`

## 参考文档

### 项目文档

- [新架构规划](新架构规划.md) - 原始架构设计
- [现代化重构架构方案](现代化重构架构方案.md) - 详细架构方案
- [ROADMAP](docs/ROADMAP.md) - 修复与演进路线图
- [PHASE3_MIGRATION_FIXES](docs/PHASE3_MIGRATION_FIXES.md) - Phase 3 修复日志
- [REFACTORING_PLAN](docs/reviews_and_fixes/05_REFACTORING_PLAN.md) - 架构重构方案
- [AGENT_GUIDE](docs/AGENT_GUIDE.md) - Agent 使用指南
- [QUICK_REFERENCE](docs/QUICK_REFERENCE.md) - 快速参考
- [CONAN_BUILD_GUIDE](docs/CONAN_BUILD_GUIDE.md) - 构建指南

### 代码审查

- [审查索引](docs/code_reviews/INDEX.md) - 所有审查报告索引
- [架构审查](docs/ARCHITECTURE_REVIEW.md)
- [代码审查报告](docs/CODE_REVIEW_REPORT.md)
- [渲染架构审查](docs/RENDERING_ARCHITECTURE_REVIEW.md)

### 外部资源

- [Vulkan 规范](https://registry.khronos.org/vulkan/)
- [Vulkan Guide](https://github.com/KhronosGroup/Vulkan-Guide)
- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [LunarG SDK](https://www.lunarg.com/vulkan-sdk/)
- [RenderDoc](https://renderdoc.org/docs/)
- [VMA 文档](https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/)

---

**最后更新**: 2026-03-15
**文档版本**: 1.1.0
**项目分支**: 2.2
# Vulkan Engine 架构问题审查报告（第二次）

**审查日期**: 2026-03-16  
**审查依据**: 上次报告 `ARCHITECTURE_ISSUES_REVIEW.md`（2026-03-15）对比最新代码  
**审查范围**: 全项目头文件 + 核心实现文件，重点对比上次报告结论  
**审查者**: WorkBuddy

---

## 零、与上次报告的总体对比

| 上次报告的问题                                   | 本次状态        | 说明                                                             |
|-------------------------------------------|-------------|----------------------------------------------------------------|
| 问题1：RenderGraph 依赖具体 Vulkan 实现            | ✅ **已改善**   | RenderGraph.hpp 不再直接 include Framebuffer                       |
| 问题2：Application 层持有裸 VkFramebuffer        | ✅ **已修复**   | 裸 `render_target_framebuffer_` 已移除                             |
| 问题3：RenderTarget 职责不完整                    | ✅ **已修复**   | RenderTarget 现在完整管理自己的 Framebuffer                             |
| 问题4：Viewport 持有 VkDescriptorSet/VkSampler | ✅ **已修复**   | 已移至 ImGuiManager 管理                                            |
| 问题5：裸 VkFramebuffer 与 RAII 混用             | ✅ **已修复**   | 统一使用 RAII 包装器                                                  |
| 问题6：缺少 Handle-Based 资源系统                  | ⚠️ **部分改善** | RenderGraphTypes.hpp 已有 ResourceHandle 骨架，但未真正接入               |
| 问题7：DeviceManager shared_ptr 到处传播         | 🔴 **未修复**  | 仍然全面传播 shared_ptr<DeviceManager>                               |
| 问题8：cleanup_resources() 手动顺序依赖            | 🔴 **未修复**  | 仍然 10+ 步手动顺序清理                                                 |
| 问题9：缺少 IResourceManager 接口                | 🔴 **未修复**  | MaterialLoader 仍然直接依赖具体类                                       |
| 问题10：RenderContext 暴露裸 Vulkan 对象          | 🔴 **未修复**  | VkRenderPass/VkFramebuffer 仍在 RenderContext 中                  |
| 问题14：绝对路径依赖                               | 🔴 **部分修复** | PathUtils 存在但 MaterialLoader / TextureLoader / main.cpp 仍有写死路径 |

---

## 一、新发现的问题

### 1.1 严重新问题（本次审查首次发现）

#### **新问题1：每帧 vkQueueWaitIdle 阻塞 — 情况比文档描述更糟**

**严重程度**: 🔴 高风险

**分布文件**:

- `src/main.cpp` line 354: 每帧 scene render 提交后立即 wait
- `src/rendering/resources/RenderTarget.cpp` line 355: resize 时 wait
- `src/rendering/material/Material.cpp` line ~597: texture upload 时 wait
- `src/rendering/resources/TextureLoader.cpp` line 367: texture upload 时 wait
- `src/editor/ImGuiManager.cpp` line 117: 初始化时 wait
- `src/vulkan/device/SwapChain.cpp` line 288: recreate 时 wait

最严重的是 **main.cpp line 354** 在主渲染循环中每帧都调用：

```cpp
// on_render() 中 — 每帧必经路径
vkQueueSubmit(device->graphics_queue(), 1, &submit_info, VK_NULL_HANDLE);
vkQueueWaitIdle(device->graphics_queue()); // ❌ 每帧 stall，CPU/GPU 无法并行
```

**影响**: 完全消除了 CPU/GPU 的流水线并行，性能上限约等于 GPU 单帧时间，无法达到目标帧率。

---

#### **新问题2：Editor 持有裸 VkCommandPool — Editor 层越权**

**严重程度**: 🔴 高风险  
**文件**: `include/editor/Editor.hpp`

```cpp
class Editor {
    VkCommandPool                command_pool_ = VK_NULL_HANDLE;  // ❌ Editor 层直接管理 Vulkan 对象
    std::vector<VkCommandBuffer> command_buffers_;                // ❌ 裸 Vulkan 句柄
};
```

Editor 是应用层组件，不应直接持有 Vulkan 命令池句柄，应通过封装的 `RenderCommandPool` RAII 对象管理。

---

#### **新问题3：ImGuiManager 命名空间声明错误**

**严重程度**: 🟡 中风险  
**文件**: `include/editor/ImGuiManager.hpp` line 100

```cpp
// ImGuiManager.hpp 末尾
} // namespace vulkan_engine::rendering   ❌ 错误！应为 vulkan_engine::editor
```

ImGuiManager 实现在 `editor` 命名空间，但头文件关闭注释写的是 `rendering`，会误导代码阅读和维护，也是潜在的 ADL（参数依赖查找）风险。

---

#### **新问题4：RenderTarget::cleanup() 内调用 vkDeviceWaitIdle**

**严重程度**: 🟡 中风险  
**文件**: `src/rendering/resources/RenderTarget.cpp` line 93

```cpp
void RenderTarget::cleanup()
{
    if (!device_) return;
    VkDevice device = device_->device();
    vkDeviceWaitIdle(device);   // ❌ 析构/resize 时全局 stall
    destroy_framebuffer();
    // ...
}
```

在 resize 路径上，每次 `resize()` 都调用 `cleanup()`，进而 `vkDeviceWaitIdle`，造成串行阻塞。正确做法是在调用 `cleanup`
之前由上层确保 GPU 空闲，而不是在资源对象内部强制 wait。

---

#### **新问题5：ViewportRenderPass 持有外部传入的裸 VkRenderPass/VkFramebuffer**

**严重程度**: 🟡 中风险  
**文件**: `include/rendering/render_graph/ViewportRenderPass.hpp`

```cpp
class ViewportRenderPass : public RenderPassBase {
    // Vulkan 资源 (外部管理，ViewportRenderPass 不拥有所有权)
    VkRenderPass  render_pass_ = VK_NULL_HANDLE; // 由外部传入 (RenderPassManager) ❌
    VkFramebuffer framebuffer_ = VK_NULL_HANDLE; // 由外部传入 (main.cpp)           ❌
};
```

**问题**: 生命周期语义完全依赖注释，没有编译期或运行期的所有权保证。如果 RenderPassManager 被提前销毁，`render_pass_`
将成为悬空句柄，极难调试。应使用 `std::weak_ptr` 或观察者句柄明确表达非所有权语义。

---

#### **新问题6：Material 使用 std::unordered_map<VkRenderPass, Pipeline> 的设计风险**

**严重程度**: 🟡 中风险  
**文件**: `include/rendering/material/Material.hpp`

```cpp
std::unordered_map<VkRenderPass, std::unique_ptr<vulkan::GraphicsPipeline>> pipelines_;
```

以 `VkRenderPass` 句柄作为 map key 是反模式：

1. 句柄是不透明整数，没有语义意义
2. 每个 RenderPass 重建后旧 Pipeline 永远不会被回收（渲染管道变更时会泄漏）
3. 应以 RenderPassKey（哈希后的格式/配置）作为键

---

#### **新问题7：RenderTarget 直接调用底层 Vulkan API 而非 VMA**

**严重程度**: 🟡 中风险  
**文件**: `src/rendering/resources/RenderTarget.cpp`

```cpp
// 手动 vkAllocateMemory / vkBindImageMemory / vkFreeMemory
VK_CHECK(vkAllocateMemory(device, &alloc_info, nullptr, &color_memory_));
VK_CHECK(vkBindImageMemory(device, color_image_, color_memory_, 0));
```

项目其他模块（如 `Buffer.hpp`/`Image.hpp`）均通过 VMA 管理内存，RenderTarget 直接调用裸 Vulkan 内存 API：

- 绕过了 VMA 的内存池、碎片整理、统计功能
- 与全局 VMA 分配器不一致
- 大量样板代码容易出错（alignment、memory type 选择等）

---

### 1.2 持续存在的旧问题

#### **问题7（续）：DeviceManager shared_ptr 全面传播**

仍然是最主要的架构问题，几乎每个系统都有：

```cpp
// 在以下类的构造函数/initialize 中均出现
RenderTarget::initialize(std::shared_ptr<vulkan::DeviceManager> device, ...)
Viewport::initialize(std::shared_ptr<vulkan::DeviceManager> device, ...)
Material::Material(std::shared_ptr<vulkan::DeviceManager> device, ...)
MaterialLoader(std::shared_ptr<vulkan::DeviceManager> device)
RenderPassManager(std::shared_ptr<DeviceManager> device)
// ... 共计 10+ 处
```

**问题**: Device 是全局唯一对象，使用 `shared_ptr` 意味着所有权语义不明确。应使用依赖注入模式（传递引用）或单例，而不是四处传递
shared_ptr。

---

#### **问题8（续）：cleanup_resources() 手动顺序依赖**

`main.cpp` 中 10 步手动清理，顺序依赖隐式而脆弱：

```cpp
void cleanup_resources() {
    // 步骤 1-11 的顺序不能打乱，但没有任何编译期保证
    render_graph_.reset();      // 1. 必须先于材质
    cube_pass_ = nullptr;       // 2.
    current_material_.reset();  // 3.
    materials_.clear();         // 4.
    material_loader_.reset();   // 5.
    vertex_buffer_.reset();     // 6.
    index_buffer_.reset();      // 7.
    mesh_.reset();              // 8.
    cmd_buffers_.clear();       // 9.
    cmd_pool_.reset();          // 10.
    framebuffer_pool_.reset();  // 11. 必须在 device 有效时
    // ... 共 15+ 步
}
```

---

#### **问题10（续）：RenderContext 暴露裸 Vulkan 对象到渲染层**

`include/rendering/render_graph/RenderGraphPass.hpp`:

```cpp
struct RenderContext {
    VkRenderPass  render_pass = VK_NULL_HANDLE;   // ❌ 裸 Vulkan 对象
    VkFramebuffer framebuffer = VK_NULL_HANDLE;   // ❌ 裸 Vulkan 对象
    std::shared_ptr<vulkan::DeviceManager> device; // ❌ 具体实现依赖
};
```

所有 RenderPass 子类（CubeRenderPass、PresentRenderPass 等）都通过 RenderContext 访问 VkRenderPass 和 VkFramebuffer，进一步加深了
Rendering 层对 Vulkan 后端的直接依赖。

---

#### **问题14（续）：路径硬编码 — PathUtils 存在但未被使用**

`PathUtils` 工具类已经存在，但多个实现文件仍然绕过它直接写死路径：

```cpp
// src/rendering/material/MaterialLoader.cpp line 21
std::string full_path = "D:/TechArt/Vulkan/materials/" + path;  // ❌ 硬编码

// src/rendering/resources/TextureLoader.cpp line 81
std::string full_path = "D:/TechArt/Vulkan/Textures/" + path;   // ❌ 硬编码

// src/main.cpp line 533
std::string obj_path = "D:/TechArt/Vulkan/model/mesh.obj";       // ❌ 硬编码
```

同时 `main.cpp` 已经正确使用了 PathUtils：

```cpp
material_loader_->set_base_directory(core::PathUtils::materials_dir().string() + "/");  // ✅
```

但 MaterialLoader::load() 内部又忽略了 `base_directory_` 成员，转而走硬编码路径，存在自相矛盾。

---

## 二、渲染架构问题详析

### 2.1 双命令流提交架构的碎片化

**文件**: `src/main.cpp` `on_render()`

当前采用两条提交路径，逻辑分散：

```
路径1: record_scene_command_buffer() -> vkQueueSubmit -> vkQueueWaitIdle  (场景渲染)
路径2: record_imgui_command_buffer() -> vkQueueSubmit + 信号量 + Fence     (GUI 渲染)
```

**问题**:

1. 场景命令流没有信号量同步，依赖裸 `vkQueueWaitIdle`
2. 两个命令池 (`cmd_pool_`) 被复用于两种不同性质的录制
3. ImGui 命令缓冲按需分配（`cmd_buffers_imgui_` 在第一帧录制时才分配），有潜在的首帧延迟

---

### 2.2 Render Graph 未真正执行

**文件**: `src/main.cpp` `initialize_render_graph()`

```cpp
render_graph_.compile();   // 调用 compile()
// ...
render_graph_.execute(cmd, ctx);  // 调用 execute()
```

但 `RenderGraph::compile()` 实现中 `build_execution_order()` 和 `generate_barriers()` 是否真正工作，还是 stub 留空，需确认。根据
AGENTS.md 描述，**Phase 4 Render Graph 真正执行**尚未开始，说明当前 execute 可能是空实现。

**风险**: 代码调用了 compile/execute，但没有实际的 barrier 插入和资源追踪，如果 CubeRenderPass 直接通过 VkRenderPass +
VkFramebuffer 渲染，Render Graph 只是一个空壳装饰器，不提供任何同步保障。

---

### 2.3 MaterialLoader::load() 接口设计缺陷

**文件**: `include/rendering/material/MaterialLoader.hpp`

```cpp
std::shared_ptr<Material> load(const std::string& path, VkRenderPass render_pass);
```

`VkRenderPass` 作为参数传入 load 是架构泄漏：

- MaterialLoader 位于 `rendering` 层
- 参数中暴露了 `VkRenderPass` Vulkan 后端句柄
- 意味着调用方（main.cpp）必须从 Vulkan 后端获取 RenderPass 句柄后才能加载材质，强制了初始化顺序

---

## 三、资源管理问题汇总

### 3.1 RenderTarget 内存管理不统一

| 系统                    | 内存管理方式                                   |
|-----------------------|------------------------------------------|
| Buffer                | VMA (`vmaCreateBuffer`)                  |
| Image (vulkan::Image) | VMA (`vmaCreateImage`)                   |
| RenderTarget          | ❌ 手动 `vkAllocateMemory` + `vkFreeMemory` |
| DepthBuffer           | 需确认                                      |

RenderTarget 作为项目核心资源，却使用了与其他资源不一致的低级 API，是一个明显的遗漏。

### 3.2 SwapChain 内部有独立的 VkRenderPass 副本

**文件**: `include/vulkan/device/SwapChain.hpp`

```cpp
VkRenderPass   default_render_pass_ = VK_NULL_HANDLE;  // SwapChain 自己持有 RenderPass
```

同时 `RenderPassManager` 也管理 RenderPass，导致两套并行的 RenderPass 管理机制：

```cpp
// main.cpp on_initialize() 中
swap_chain->create_render_pass_with_depth(depth_buffer_->format());          // 让 SwapChain 创建
VkRenderPass present_render_pass = render_pass_manager_->get_present_render_pass_with_depth(...);  // 又通过 Manager 创建
```

这导致同一个 RenderPass 可能被创建两次，两个句柄并存，后续使用哪个取决于调用路径，极难维护。

---

## 四、线程安全评估

### 4.1 现状

仅 `Material::uniform_mutex_` 一处有同步保护，整体项目**无线程安全设计**。

### 4.2 主要风险点

| 位置                         | 风险                                             |
|----------------------------|------------------------------------------------|
| `logger::info/warn/error`  | 全局函数，无锁保护，多线程调用会数据竞争                           |
| `PathUtils` 静态成员变量         | `project_root_`、`search_paths_` 无锁读写           |
| `Material::pipelines_` map | 虽有 mutex 保护 uniform buffer，但 pipelines_ 的读写无保护 |
| `RenderTarget::cleanup()`  | 可能被 resize 回调和主线程同时访问                          |
| ImGui 帧数据                  | `begin_frame()` / `end_frame()` 之间无并发保护        |

---

## 五、构建系统分析

### 5.1 CMake 模块结构良好

相比上次报告，本次确认 CMake 模块化程度较高：

- `Core.cmake`, `Platform.cmake`, `Rendering.cmake`, `Vulkan.cmake`, `Editor.cmake`, `Application.cmake` 分层清晰
- 采用 `create_vulkan_module` 宏统一管理

### 5.2 Rendering 模块链接了 ImGui — 职责违规

**文件**: `CMake/Modules/Rendering.cmake`

```cmake
# ImGui for SceneViewport ( Editor UI )
if (imgui_FOUND)
    target_link_libraries(VulkanEngineRendering PUBLIC imgui::imgui)  # ❌ Rendering 不应依赖 Editor GUI 库
```

Rendering 层为了 SceneViewport 而依赖 ImGui，违反了"渲染层不依赖 UI 框架"的架构原则。ImGui 依赖应仅出现在 Editor 模块。

---

## 六、问题优先级总表

| 编号  | 问题                                            | 严重程度 | 类型  | 是否新发现 |
|-----|-----------------------------------------------|------|-----|-------|
| N1  | 每帧 `vkQueueWaitIdle` 阻塞主渲染循环                  | 🔴 高 | 性能  | ✅ 新   |
| N2  | Editor 持有裸 VkCommandPool                      | 🔴 高 | 架构  | ✅ 新   |
| P7  | `DeviceManager` shared_ptr 全面传播               | 🔴 高 | 设计  | 遗留    |
| P8  | 手动 cleanup 顺序依赖                               | 🔴 高 | 可靠性 | 遗留    |
| N3  | ImGuiManager 命名空间注释错误                         | 🟡 中 | 正确性 | ✅ 新   |
| N4  | RenderTarget::cleanup 内 vkDeviceWaitIdle      | 🟡 中 | 性能  | ✅ 新   |
| N5  | ViewportRenderPass 裸句柄生命周期不安全                 | 🟡 中 | 可靠性 | ✅ 新   |
| N6  | Material 以 VkRenderPass 句柄为 map key           | 🟡 中 | 设计  | ✅ 新   |
| N7  | RenderTarget 不使用 VMA                          | 🟡 中 | 一致性 | ✅ 新   |
| N8  | SwapChain 与 RenderPassManager 双轨管理 RenderPass | 🟡 中 | 架构  | ✅ 新   |
| P10 | RenderContext 暴露裸 Vulkan 对象                   | 🟡 中 | 架构  | 遗留    |
| P14 | PathUtils 存在但 3 处实现仍硬编码路径                     | 🟡 中 | 可维护 | 遗留    |
| N9  | Rendering 模块 CMake 链接 ImGui                   | 🟡 中 | 架构  | ✅ 新   |
| P9  | 缺少 IResourceManager 抽象接口                      | 🟢 低 | 扩展性 | 遗留    |

---

## 七、修复建议

### 优先级 1（立即修复）

**修复 N1 — 去除每帧 vkQueueWaitIdle**

```cpp
// 错误（当前）
vkQueueSubmit(device->graphics_queue(), 1, &submit_info, VK_NULL_HANDLE);
vkQueueWaitIdle(device->graphics_queue()); // ❌

// 正确
// 使用 Timeline Semaphore 或在 scene_submit 中传入适当的信号量
// 让 GUI 提交的 wait semaphore 等待 scene 完成
VkSemaphore scene_done_semaphore = frame_sync_->get_scene_done_semaphore();
submit_info.signalSemaphoreCount = 1;
submit_info.pSignalSemaphores    = &scene_done_semaphore;
vkQueueSubmit(device->graphics_queue(), 1, &submit_info, VK_NULL_HANDLE);
// GUI submit 等待 scene_done_semaphore
```

**修复 P14 — 统一使用 PathUtils**

```cpp
// MaterialLoader.cpp
std::string full_path = (std::filesystem::path(base_directory_) / path).string();
// 或
std::string full_path = core::PathUtils::resolve_material(path).string();
```

**修复 N3 — 命名空间注释**

```cpp
} // namespace vulkan_engine::editor   // ✅ 修正
```

### 优先级 2（近期修复）

**修复 N4 — RenderTarget::cleanup() 移除 vkDeviceWaitIdle**

调用方在使用 RenderTarget 前应负责 GPU 同步，而不是资源对象自己 stall。

**修复 N8 — 统一 RenderPass 管理**

选择一个权威来源（推荐 `RenderPassManager`），删除 SwapChain 内部的 `default_render_pass_` 或将其委托给 RenderPassManager
管理。

**修复 N2 — Editor 使用 RAII 命令池**

```cpp
// 替换
VkCommandPool                command_pool_ = VK_NULL_HANDLE;  // ❌
// 为
std::unique_ptr<vulkan::RenderCommandPool> command_pool_;     // ✅
```

**修复 N7 — RenderTarget 改用 VMA**

参考 `vulkan::Image` 的实现，将 `vkAllocateMemory/vkBindImageMemory/vkFreeMemory` 替换为
`vmaCreateImage/vmaDestroyImage`。

### 优先级 3（中期重构）

**修复 N6 — Material 更换 pipeline map key**

```cpp
// 改用 RenderPassKey 而不是裸句柄
std::unordered_map<vulkan::RenderPassKey, 
                   std::unique_ptr<vulkan::GraphicsPipeline>,
                   vulkan::RenderPassKeyHash> pipelines_;
```

**修复 N9 — Rendering 模块移除 ImGui 依赖**

将 ImGui-specific 的渲染逻辑（SceneViewport 相关）移入 Editor 模块，Rendering 模块只提供纯粹的渲染抽象。

**修复 P8 — 使用层次化所有权简化 cleanup**

引入渲染资源所有权树，父节点析构时自动清理子节点，消除手动顺序管理。

---

## 八、总结

相较于上次审查（2026-03-15），本次代码在**以下方面有实质改善**：

- ✅ Application 层不再持有裸 `VkFramebuffer`
- ✅ RenderTarget 完整管理自己的 Framebuffer（RAII）
- ✅ Viewport 不再持有 VkDescriptorSet/VkSampler
- ✅ PathUtils 工具类已实现

但**仍然存在或新增了以下核心问题**：

- 🔴 主渲染循环每帧 `vkQueueWaitIdle`，严重影响性能
- 🔴 DeviceManager shared_ptr 大规模传播，所有权语义不清
- 🔴 3 处实现文件绕过 PathUtils 硬编码路径，部分修复效果被抵消
- 🟡 RenderTarget 使用非 VMA 内存管理与全局策略不一致
- 🟡 SwapChain 与 RenderPassManager 双轨管理导致 RenderPass 冗余创建

**当前最紧迫的单点修复**：去除 `on_render()` 中的每帧 `vkQueueWaitIdle`，改为信号量同步，这是影响运行时性能的最大瓶颈。

---

**审查完成时间**: 2026-03-16  
**参考文档**: `ARCHITECTURE_ISSUES_REVIEW.md` (2026-03-15), `AGENTS.md`, `docs/ROADMAP.md`

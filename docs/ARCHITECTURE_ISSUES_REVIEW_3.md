# Vulkan Engine 架构问题深度审查报告（第三次）

**审查日期**: 2026-03-18  
**审查依据**: 第二次报告 `ARCHITECTURE_ISSUES_REVIEW_2.md`（2026-03-16）对比最新代码  
**审查范围**: 全项目头文件 + 核心实现文件；重点关注同步子系统、资源管理架构、层间耦合  
**触发变更**: `include/vulkan/sync/Synchronization.hpp`（格式调整，2 处空白字符变化）  
**审查者**: WorkBuddy

---

## 零、本次 Git Diff 分析

本次 `Synchronization.hpp` 的唯一变更为**纯空白字符调整**：

```diff
-            uint32_t current_frame() const { return current_frame_; }
-
+            uint32_t current_frame() const { return current_frame_; }
+
-            std::vector<std::unique_ptr<Semaphore>> acquire_semaphores_;      // vkAcquireNextImageKHR
+            std::vector<std::unique_ptr<Semaphore>> acquire_semaphores_;         // vkAcquireNextImageKHR
```

**结论：本次 diff 不引入任何功能或架构变化。** 触发本次报告的是持续跟踪上次遗留的未修复问题，以及对同步子系统的专项深度审查。

---

## 一、与第二次报告的问题状态对比

| 编号      | 问题描述                                           | 上次状态   | 本次状态       | 说明                                                  |
|---------|------------------------------------------------|--------|------------|-----------------------------------------------------|
| **N1**  | 主循环每帧 `vkQueueWaitIdle`                        | 🔴 高风险 | ✅ **已修复**  | 改用 `scene_finished_semaphore` 信号量链，CPU/GPU 可并行      |
| **N2**  | Editor 持有裸 `VkCommandPool`                     | 🔴 高风险 | ✅ **已修复**  | `Editor.hpp` 不再有 `VkCommandPool` 成员                 |
| **N3**  | `ImGuiManager.hpp` 命名空间注释错误                    | 🟡 中风险 | ⚠️ **待确认** | 需再次验证                                               |
| **N4**  | `RenderTarget::cleanup()` 内 `vkDeviceWaitIdle` | 🟡 中风险 | 🟡 **仍存在** | resize 路径每次触发，见下文分析                                 |
| **N5**  | `CubeRenderPass::Config` 裸指针 `vulkan::Buffer*` | 🟡 中风险 | 🟡 **仍存在** | 无所有权语义                                              |
| **N6**  | `BufferHandle`/`ImageHandle` 独立 static ID 计数器  | 🟡 中风险 | 🟡 **仍存在** | ID 空间隔离，潜在混用                                        |
| **N7**  | `TimelineSemaphore` 为空壳实现                      | 🟡 中风险 | 🟡 **仍存在** | 3 个方法均为 placeholder，无实际功能                           |
| **P7**  | `shared_ptr<DeviceManager>` 全面传播               | 🔴 高风险 | 🔴 **仍存在** | 10+ 子系统传播，所有权语义不明                                   |
| **P8**  | `cleanup_resources()` 手动顺序清理                   | 🔴 高风险 | 🔴 **仍存在** | 11 步手动清理，无编译期保证                                     |
| **P10** | `RenderContext` 暴露裸 Vulkan 对象                  | 🟡 中风险 | 🟡 **仍存在** | `VkRenderPass`/`VkFramebuffer` 穿透到 Rendering 层      |
| **P14** | 3 处硬编码绝对路径，绕过 PathUtils                        | 🟡 中风险 | 🟡 **仍存在** | `main.cpp`、`MaterialLoader.cpp`、`TextureLoader.cpp` |

**本次修复率**: 2/11（N1 ✅、N2 ✅），修复进度 18%

---

## 二、同步子系统专项深度审查

### 2.1 `FrameSyncManager` 架构评估

**正面**：设计整洁，pure per-frame 架构，消除了 per-image 数组的混淆。信号量语义清晰：

```
acquire → [scene render] → scene_finished → [ImGui render] → render_finished → present
```

**问题 S1：`FrameSyncManager` 仍然依赖 `shared_ptr<DeviceManager>`**

`FrameSyncManager` 在构造时接受 `shared_ptr<DeviceManager>` 并存储为成员，但析构函数 `= default`，导致子对象 `Fence`/
`Semaphore` 的析构依赖 `device_` 在其之前仍然有效。

```cpp
// Synchronization.hpp:136
~FrameSyncManager() = default;  // ← 依赖成员析构顺序

// 内部成员析构顺序（按声明逆序）：
// 4. render_finished_semaphores_ → Semaphore::~Semaphore() 调用 device_->device()
// 3. scene_finished_semaphores_
// 2. acquire_semaphores_
// 1. frame_fences_
// 0. frame_count_, current_frame_
// device_ 最后析构 ✓ (shared_ptr 引用计数)
```

由于 `device_` 在成员列表中排在 vector 之前（行 180），析构顺序是**正确的**——但这是一个**隐式的、脆弱的顺序依赖**。如果有人将
`device_` 的声明位置移到 vectors 之后，就会产生 use-after-free。

**建议 S1**：在析构函数中明确先销毁向量，再释放 device：

```cpp
~FrameSyncManager() {
    frame_fences_.clear();
    acquire_semaphores_.clear();
    scene_finished_semaphores_.clear();
    render_finished_semaphores_.clear();
    // device_ shared_ptr 最后自动 release
}
```

---

### 2.2 `TimelineSemaphore` 空壳问题（N7 深度分析）

**严重程度**: 🟡 中风险（接口欺骗性）

`TimelineSemaphore` 继承自 `Semaphore`，但在构造时完全忽略 `initial_value`，且 3 个核心方法均为空操作：

```cpp
// Synchronization.cpp:123
TimelineSemaphore::TimelineSemaphore(std::shared_ptr<DeviceManager> device, uint64_t /*initial_value*/)
    : Semaphore(std::move(device))  // 创建的是普通 binary semaphore！
{ }

void TimelineSemaphore::signal(uint64_t /*value*/) { /* 空操作 */ }
void TimelineSemaphore::wait(uint64_t /*value*/, uint64_t /*timeout*/) { /* 空操作 */ }
uint64_t TimelineSemaphore::get_value() const { return 0; } // 永远返回 0
```

**核心问题**：`SynchronizationManager::create_timeline_semaphore()` 对外暴露这个类型，任何调用者都会得到一个*
*看起来正常但实际无效的对象**。这是一种"静默失败"——调用 `signal()`/`wait()` 不报错，但同步约束完全没有生效。

**正确实现需要 `VkSemaphoreTypeCreateInfo`（Vulkan 1.2+ 或 `VK_KHR_timeline_semaphore`）**：

```cpp
// 正确的 Timeline Semaphore 创建（需要 Vulkan 1.2+）
VkSemaphoreTypeCreateInfo timeline_info{};
timeline_info.sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
timeline_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
timeline_info.initialValue  = initial_value;

VkSemaphoreCreateInfo semaphore_info{};
semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
semaphore_info.pNext = &timeline_info;  // 链接到 timeline 扩展
```

**建议 S2**：有三条路径：

1. **实现它**：检查设备特性 `timelineSemaphore`，在构造时使用 `pNext` 链创建真正的 timeline semaphore（推荐，项目定位 Vulkan
   1.3+）。
2. **禁用它**：将 `TimelineSemaphore` 标记为 `[[deprecated]]`，删除 `create_timeline_semaphore()`，防止误用。
3. **断言失败**：在 3 个方法中加 `assert(false)` 或抛出异常，至少让误用立即可见。

---

### 2.3 `SynchronizationManager::submit_with_sync` 的设计缺陷

**严重程度**: 🟡 中风险

```cpp
// Synchronization.cpp:281
for (const auto& sem : wait_semaphores)
{
    vk_wait_semaphores.push_back(sem->handle());
    wait_stages.push_back(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT); // ← 硬编码
}
```

所有等待信号量的 `pWaitDstStageMask` 都被硬编码为 `VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT`。这对于颜色输出等待是正确的，但对于：

- Compute → Graphics 依赖：应使用 `VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT`
- Transfer 完成依赖：应使用 `VK_PIPELINE_STAGE_TRANSFER_BIT`
- 深度预通道：应使用 `VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT`

**建议 S3**：将 `wait_stages` 作为参数传入，或为每个等待信号量配对一个 stage flag：

```cpp
struct WaitInfo {
    std::shared_ptr<Semaphore> semaphore;
    VkPipelineStageFlags       stage;
};

void submit_with_sync(VkQueue queue, VkCommandBuffer cmd,
                      const std::vector<WaitInfo>& wait_infos = {},
                      ...);
```

---

### 2.4 `Fence::wait()` 忽略返回值

**严重程度**: 🟡 中风险

```cpp
void Fence::wait(uint64_t timeout)
{
    vkWaitForFences(device_->device(), 1, &fence_, VK_TRUE, timeout); // ← 返回值丢失
}
```

`vkWaitForFences` 在超时时返回 `VK_TIMEOUT`，在设备丢失时返回 `VK_ERROR_DEVICE_LOST`。完全忽略返回值意味着这两种情况都会静默通过，之后对
`fence_` 的操作可能产生未定义行为。

同样的问题存在于 `SynchronizationManager::wait_for_fences()`、`Event::set()`、`Event::reset()`。

**建议 S4**：

```cpp
void Fence::wait(uint64_t timeout)
{
    VkResult result = vkWaitForFences(device_->device(), 1, &fence_, VK_TRUE, timeout);
    if (result == VK_TIMEOUT) {
        throw std::runtime_error("Fence wait timed out");
    }
    if (result != VK_SUCCESS) {
        throw VulkanError(result, "vkWaitForFences failed", __FILE__, __LINE__);
    }
}
```

---

### 2.5 `Event` 类的现实使用价值问题

**严重程度**: 🟢 低风险（设计评估）

`VkEvent` 在 Vulkan 中主要用于**管线内同步**（`vkCmdSetEvent`/`vkCmdWaitEvents`），但当前 `Event` 类提供的 API（`set()`/
`reset()`/`is_set()`）是基于 **CPU 端**的 `vkSetEvent`/`vkResetEvent`/`vkGetEventStatus`。

在当前项目的渲染架构中（命令缓冲 + 信号量链），`VkEvent` 的使用场景极为有限。这个类目前是死代码——
`SynchronizationManager::create_event()` 有实现，但项目中没有任何地方调用它。

**建议 S5**：标记为 `[[maybe_unused]]` 或添加注释说明用途。若 Phase 5 的多线程录制需要 pipeline event，再完善。

---

## 三、持续未修复问题深度分析

### 3.1 `shared_ptr<DeviceManager>` 传播问题（P7 延伸分析）

本次统计共有 **13 个类**的构造函数接受 `shared_ptr<DeviceManager>`：

| 类                        | 文件                        |
|--------------------------|---------------------------|
| `Fence`                  | `Synchronization.hpp:14`  |
| `Semaphore`              | `Synchronization.hpp:38`  |
| `TimelineSemaphore`      | `Synchronization.hpp:57`  |
| `Event`                  | `Synchronization.hpp:68`  |
| `SynchronizationManager` | `Synchronization.hpp:91`  |
| `FrameSyncManager`       | `Synchronization.hpp:134` |
| `Buffer`                 | `Buffer.hpp`              |
| `Image`                  | `Image.hpp`               |
| `DepthBuffer`            | `DepthBuffer.hpp`         |
| `RenderTarget`           | `RenderTarget.hpp`        |
| `Viewport`               | `Viewport.hpp`            |
| `MaterialLoader`         | `MaterialLoader.hpp`      |
| `CommandBufferManager`   | `Device.hpp`              |

**核心问题**：`shared_ptr` 暗示**共享所有权**，但 `DeviceManager` 实际上是**单例级别的全局资源**，不存在多个消费者共同"
拥有"它的语义。这里应使用 `std::reference_wrapper<DeviceManager>` 或传 `VkDevice` 裸句柄（更接近 Vulkan 的使用惯例）。

`Fence`/`Semaphore`/`Event` 这类底层 Vulkan 对象，只需要 `VkDevice` 句柄来调用 `vkDestroyFence` 等，完全不需要持有
`DeviceManager` 的生命周期。

**建议 P7-A**（渐进式）：底层对象（Fence/Semaphore/Event/Buffer/Image）改为接受裸 `VkDevice`：

```cpp
class Fence {
public:
    Fence(VkDevice device, bool signaled = false);
    ~Fence() { if (fence_ != VK_NULL_HANDLE) vkDestroyFence(device_, fence_, nullptr); }
private:
    VkDevice device_; // 非拥有，生命周期由上层保证
    VkFence  fence_ = VK_NULL_HANDLE;
};
```

---

### 3.2 `cleanup_resources()` 11 步手动顺序（P8 延伸分析）

```cpp
// main.cpp:853-925 — 11 步手动清理，顺序依赖编写者的人工推断
void cleanup_resources() {
    vkDeviceWaitIdle(vk_device);          // 步骤 0：等待 GPU
    render_graph_.reset();                // 步骤 1
    cube_pass_ = nullptr;                 // 步骤 2：观察指针置空
    render_target_->destroy_framebuffer();// 步骤 3
    render_target_render_pass_ = VK_NULL_HANDLE; // 步骤 4：裸句柄置空
    current_material_.reset();            // 步骤 5
    materials_.clear();
    material_loader_.reset();             // 步骤 6
    vertex_buffer_.reset();               // 步骤 7
    index_buffer_.reset();
    mesh_.reset();
    cmd_buffers_.clear();                 // 步骤 8
    cmd_buffers_imgui_.clear();
    cmd_pool_.reset();
    framebuffer_pool_.reset();            // 步骤 9
    frame_sync_.reset();                  // 步骤 10
    viewport_.reset();                    // 步骤 11
    render_target_.reset();
    depth_buffer_.reset();
    render_pass_manager_.reset();
    camera_controller_.reset();
    camera_.reset();
}
```

**三个根本问题**：

1. **`cube_pass_` 是观察指针**（`rendering::CubeRenderPass* cube_pass_ = nullptr`），当 `render_graph_.reset()`
   后它指向悬空内存，必须在之后立即置空，靠注释维护这个约束不可靠。

2. **`render_target_render_pass_`（`VkRenderPass`）是裸句柄**，不归 `cleanup_resources()` 销毁——它由 `render_pass_manager_`
   的缓存管理，但裸句柄没有任何机制保证这一点。如果顺序写错（先 reset `render_pass_manager_`，再用
   `render_target_render_pass_`），则 use-after-free。

3. **该函数不是在析构函数中调用的**（见 `main.cpp:1034`，在 `app->shutdown()` 中手动调用），这意味着如果 `initialize()` 成功但
   `shutdown()` 未被调用（如异常路径），资源就会泄漏。

**建议 P8-A**：`cube_pass_` 改为 `std::weak_ptr` 或从 RenderGraph 查询：

```cpp
// 不要存储观察指针，从 render_graph_ 查询
rendering::CubeRenderPass* cube_pass() const {
    return render_graph_ ? render_graph_->find_pass<CubeRenderPass>("cube") : nullptr;
}
```

**建议 P8-B**：`render_target_render_pass_` 改为通过 RenderTarget 自身管理，不在 Application 层持有裸 `VkRenderPass`。

---

### 3.3 `process_viewport_resize` 内 `vkDeviceWaitIdle`（N4 延伸）

```cpp
// main.cpp:997-1000
void process_viewport_resize(uint32_t width, uint32_t height)
{
    if (auto device = device_manager()) {
        vkDeviceWaitIdle(device->device()); // ← 每次 resize 都停止整个 GPU
    }
    viewport_->apply_pending_resize();
    create_render_target_framebuffer();
}
```

`vkDeviceWaitIdle` 会等待**所有队列（graphics + present +
compute）**的所有操作完成，代价远高于必要。Viewport resize 只需要等待**依赖该 RenderTarget 的命令缓冲**完成，应使用
`frame_fence`（已有）。

**建议 N4-A**：

```cpp
void process_viewport_resize(uint32_t width, uint32_t height)
{
    // 只需等待当前帧 fence，而非整个设备
    if (frame_sync_) {
        frame_sync_->wait_and_reset_current_frame_fence();
    }
    viewport_->apply_pending_resize();
    create_render_target_framebuffer();
}
```

---

### 3.4 `RenderContext` 裸 Vulkan 句柄穿透（P10 延伸）

```cpp
// RenderGraphPass.hpp:24-38
struct RenderContext {
    uint32_t frame_index = 0;
    uint32_t image_index = 0;
    uint32_t width       = 0;
    uint32_t height      = 0;
    VkRenderPass  render_pass = VK_NULL_HANDLE;  // ← Vulkan Backend 泄漏到 Rendering 层
    VkFramebuffer framebuffer = VK_NULL_HANDLE;  // ← 同上
    std::shared_ptr<vulkan::DeviceManager> device; // ← 具体实现依赖
};
```

`RenderContext` 位于 `rendering/` 命名空间，但持有 `vulkan/` 层的裸对象，使得 Rendering 层无法脱离 Vulkan 实现测试或替换。

**建议 P10-A**：将 `RenderContext` 分为两层：

```cpp
// Rendering 层：抽象 context（不含裸句柄）
struct RenderContext {
    uint32_t frame_index = 0;
    uint32_t image_index = 0;
    uint32_t width       = 0;
    uint32_t height      = 0;
};

// Vulkan 层：具体执行 context（含裸句柄）
struct VulkanRenderContext : public RenderContext {
    VkRenderPass  render_pass = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VkDevice      device      = VK_NULL_HANDLE;
};
```

---

## 四、新发现问题

### 4.1 `on_resize()` 重建 `FrameSyncManager` 的竞争风险

**严重程度**: 🔴 高风险

```cpp
// main.cpp:493-495
frame_sync_.reset();  // ← 销毁所有 fence/semaphore
frame_sync_ = std::make_unique<vulkan::FrameSyncManager>(device, 2);
```

`on_resize()` 在 `vkAcquireNextImageKHR` 返回 `VK_ERROR_OUT_OF_DATE_KHR` 后被调用，此时当前帧的信号量（`acquire_semaphore`
）已经被 `vkAcquireNextImageKHR` 消耗，但 GPU 可能仍在使用上一帧的 `render_finished_semaphore` 或
`scene_finished_semaphore`。

直接 `reset()` 销毁这些信号量，会导致 GPU 在 `vkQueueSubmit` 时引用已销毁的 `VkSemaphore` 句柄——这是一个**验证层会明确报告的
use-after-free**。

正确做法应该是在 `reset()` 之前等待队列空闲：

```cpp
void on_resize(uint32_t /*w*/, uint32_t /*h*/) override
{
    auto device = device_manager();
    // 在重建同步对象之前，确保 GPU 不再使用它们
    vkDeviceWaitIdle(device->device()); // ← resize 路径允许一次性阻塞
    
    frame_sync_.reset();
    frame_sync_ = std::make_unique<vulkan::FrameSyncManager>(device, 2);
    // ...
}
```

当前代码在 `on_resize()` 内**没有任何等待**（`vkDeviceWaitIdle` 在 `cleanup_resources()` 中，不在 `on_resize()`
中），使得信号量重建存在竞争风险。

---

### 4.2 `swap_chain->image_count()` 与 `FrameSyncManager(device, 2)` 硬编码不一致

**严重程度**: 🟡 中风险

```cpp
// main.cpp:495
frame_sync_ = std::make_unique<vulkan::FrameSyncManager>(device, 2); // ← 硬编码 2
```

`FrameSyncManager` 的 frame_count 硬编码为 2，但实际的 `swap_chain->image_count()` 可能是 2 或 3（取决于 VSync
模式和驱动）。如果交换链有 3 张图像，但 frame_sync 只有 2 个 frame slot，当 `current_frame_` 在 0/1
之间轮转时，image_index（0/1/2）可能超出 frame_sync 的数组范围。

虽然当前代码中 `image_index` 和 `frame_index` 是分开使用的（这是 per-frame 架构的正确做法），但仍应该统一
MAX_FRAMES_IN_FLIGHT 常量：

```cpp
constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
// ...
frame_sync_ = std::make_unique<vulkan::FrameSyncManager>(device, MAX_FRAMES_IN_FLIGHT);
```

---

### 4.3 `record_imgui_command_buffer` 中的 `cmd_buffers_imgui_.empty()` 懒初始化反模式

**严重程度**: 🟡 中风险

```cpp
// main.cpp:705-708
if (cmd_buffers_imgui_.empty())
{
    cmd_buffers_imgui_ = cmd_pool_->allocate(2); // 懒初始化
}
```

这是一个**运行时检测的资源初始化**，放在热路径（每帧必经）中。这不仅有轻微性能开销，更重要的是：在 `on_resize()` 中执行了
`cmd_buffers_imgui_.clear()`，下一帧会在这里重新分配，但 `cmd_pool_` 是否有足够余量？重新分配的命令缓冲是否需要提交到
`vkDeviceWaitIdle` 之后的新状态？

**建议**：在初始化时分配，在 resize 后显式重分配，去除热路径中的条件检查。

---

### 4.4 `viewport_render_pass_` 成员变量未在 cleanup 中清理

**严重程度**: 🟡 中风险

```cpp
// main.cpp:933
VkRenderPass viewport_render_pass_ = VK_NULL_HANDLE; // ← 成员变量
```

`cleanup_resources()` 中清理了 `render_target_render_pass_`，但没有处理 `viewport_render_pass_`。这个裸句柄指向
`render_pass_manager_` 管理的 `VkRenderPass`——`render_pass_manager_.reset()` 会销毁它，之后 `viewport_render_pass_`
成为悬空句柄。

虽然在当前代码流程中 `viewport_render_pass_` 在 `render_pass_manager_` 销毁后不再被使用，但这是一个靠执行顺序维护的隐式约束，危险。

---

## 五、优先级汇总与修复路线

### 🔴 高风险（影响稳定性，需立即修复）

| 编号        | 问题                                                                           | 修复代价   | 影响范围                   |
|-----------|------------------------------------------------------------------------------|--------|------------------------|
| **NEW-A** | `on_resize()` 重建 `FrameSyncManager` 前缺少 `vkDeviceWaitIdle`，可能 use-after-free | 低（加一行） | `main.cpp:on_resize()` |
| **P7**    | `shared_ptr<DeviceManager>` 全面传播，语义混乱                                        | 高（重构）  | 全项目 13 个类              |
| **P8**    | `cleanup_resources()` 手动顺序清理，无编译期安全保证                                        | 中      | `main.cpp`             |

### 🟡 中风险（影响正确性或可维护性，近期修复）

| 编号        | 问题                                                               | 修复代价            | 影响范围                                                  |
|-----------|------------------------------------------------------------------|-----------------|-------------------------------------------------------|
| **S1**    | `FrameSyncManager` 析构顺序隐式依赖                                      | 低（加显式析构体）       | `Synchronization.hpp/cpp`                             |
| **S2**    | `TimelineSemaphore` 空壳实现，静默失败                                    | 中（实现或明确禁用）      | `Synchronization.cpp`                                 |
| **S3**    | `submit_with_sync` 硬编码 wait stage                                | 低（修改接口）         | `Synchronization.cpp`                                 |
| **S4**    | `Fence::wait()` 忽略 Vulkan 返回值                                    | 低（加错误检查）        | `Synchronization.cpp`                                 |
| **N4**    | `process_viewport_resize` 内 `vkDeviceWaitIdle`（过度等待）             | 低（改用 fence）     | `main.cpp`                                            |
| **NEW-B** | `FrameSyncManager` frame_count 硬编码 2，缺 `MAX_FRAMES_IN_FLIGHT` 常量 | 低               | `main.cpp`                                            |
| **NEW-C** | `viewport_render_pass_` 未在 cleanup 中显式置空                         | 低               | `main.cpp`                                            |
| **P10**   | `RenderContext` 暴露裸 Vulkan 句柄                                    | 中（分层重构）         | `RenderGraphPass.hpp`                                 |
| **P14**   | 3 处硬编码绝对路径                                                       | 低（改用 PathUtils） | `main.cpp`, `MaterialLoader.cpp`, `TextureLoader.cpp` |

### 🟢 低风险（可维护性问题，有机会处理）

| 编号         | 问题                                    | 修复代价   |
|------------|---------------------------------------|--------|
| **S5**     | `Event` 类为死代码，缺少使用文档                  | 低（加注释） |
| **NEW-C2** | `cmd_buffers_imgui_` 懒初始化在热路径中        | 低      |
| **N5**     | `CubeRenderPass::Config` 裸指针          | 中      |
| **N6**     | `BufferHandle`/`ImageHandle` 独立 ID 空间 | 低      |

---

## 六、总体架构健康度评估

与第二次报告（2026-03-16）相比：

| 维度         | 第一次（03-15）      | 第二次（03-16）      | 本次（03-18）                | 趋势 |
|------------|-----------------|-----------------|--------------------------|----|
| **同步正确性**  | 🔴 严重（每帧 stall） | 🔴 严重（每帧 stall） | 🟡 中（已修复主循环，仍有边缘问题）      | ↑  |
| **资源管理**   | 🔴 严重（裸句柄混用）    | 🟡 中（RAII 改善）   | 🟡 中（稳定）                 | →  |
| **层间耦合**   | 🔴 严重           | 🟡 中            | 🟡 中（RenderContext 仍有问题） | →  |
| **生命周期安全** | 🟡 中            | 🟡 中            | 🟡 中（新发现 resize 竞争）      | →  |
| **可维护性**   | 🔴 严重           | 🟡 中            | 🟡 中                     | →  |

**综合评分**：**58/100**（第二次：52/100，有所改善，主要来自 N1/N2 修复）

**核心阻碍**：项目向 Phase 4（真实 RenderGraph 执行）推进的最大阻碍仍然是 **P7（DeviceManager 传播）** 和 **P8（手动清理）**
——这两个问题随着 Pass 数量增加会线性恶化，应在 Phase 4 开始前解决。

---

## 七、下一步行动建议（优先级排序）

1. **立即**：修复 `on_resize()` 中的 `vkDeviceWaitIdle` 缺失（NEW-A，1 行改动，避免 crash）
2. **立即**：修复 `Fence::wait()` 等返回值丢失（S4，防止设备丢失时静默通过）
3. **本周**：决策 `TimelineSemaphore`——实现或明确标记为不可用（S2）
4. **Phase 4 前**：解决 `submit_with_sync` 硬编码 wait stage（S3，否则无法支持 Compute pass）
5. **Phase 4 前**：分离 `RenderContext`（P10，否则 RenderGraph 无法脱离 Vulkan 层）
6. **中期**：`DeviceManager` 传播重构（P7，可以渐进式从底层 Fence/Semaphore/Buffer 开始）

---

*报告生成：WorkBuddy | 基于 git diff + 全量代码扫描 | 2026-03-18*

# 代码审查报告 — main.cpp / EditorApplication

**文件路径**

- `src/main.cpp`

**审查日期**: 2026-03-15
**严重程度图例**: 🔴 高风险 | 🟡 中风险 | 🟢 低风险

---

## 功能摘要

`main.cpp` 定义了 `EditorApplication`（继承自 `ApplicationBase`），是整个引擎的**顶层组装类**，负责串联所有子系统并驱动主循环。

| 阶段                                 | 关键操作                                                                                                                                                                           |
|------------------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| **初始化** (`on_initialize`)          | 创建 DepthBuffer → RenderPassManager → Viewport/RenderTarget → Editor → FrameSyncManager → FramebufferPool → CommandPool → 加载 Mesh → 初始化 Material → 初始化 RenderGraph → 初始化 Camera |
| **更新** (`on_update`)               | 更新 FPS → 更新 CameraController → 处理 M 键材质切换 → 更新 Editor Stats                                                                                                                    |
| **渲染** (`on_render`)               | 等待 Fence → 获取交换链图像 → ImGui 新帧 → 录制场景命令缓冲 → 提交场景渲染 → 录制 ImGui 命令缓冲 → 提交 ImGui 渲染 → Present                                                                                      |
| **窗口 Resize** (`on_window_resize`) | 重建 SwapChain → DepthBuffer → RenderPass → RenderTarget → Viewport → FramebufferPool → FrameSyncManager → 重建 ImGui                                                              |

文件顶部还定义了用于测试的立方体顶点/索引数组（全局 `const` 变量）。

---

## 潜在问题

### 🔴 高风险

#### 1. 场景渲染提交使用 `vkQueueWaitIdle` 阻塞，破坏帧间并行性

```cpp
vkQueueSubmit(device->graphics_queue(), 1, &submit_info, VK_NULL_HANDLE);
vkQueueWaitIdle(device->graphics_queue()); // ← 每帧完全阻塞图形队列
logger::info("Scene render completed");    // ← 每帧都打印 info 日志
```

**影响**：CPU 每帧都等待 GPU 完成场景渲染才能继续提交 ImGui 渲染。双缓冲 FrameSyncManager 形同虚设，实际帧率上限被 GPU
单帧耗时严格限制，无法利用 CPU/GPU 并行流水线。

**正确方式**：提交场景渲染时添加 signal semaphore，ImGui 渲染等待该 semaphore，而不是 `vkQueueWaitIdle`。

#### 2. 每帧都向 `logger::info` 输出渲染进度日志，严重影响性能

```cpp
// on_render() 中每帧执行：
logger::info("Submitting scene render command buffer...");
logger::info("Scene render completed");
```

以及 `initialize_materials()` 中打印了 `render_target_render_pass_` 的十六进制地址——虽然只在初始化时执行一次，但
`logger::info` 在整个引擎中完全没有级别过滤（参见 Logger 报告），这两行在 60fps 时每秒产生 120 条日志，对性能和调试信息可读性影响极大。

#### 3. `on_window_resize()` 代码格式严重损坏（极端缩进嵌套）

```cpp
void on_window_resize(const application::WindowResizeEvent& event) override
{
        uint32_t width = event.width;
        // ...
                        if (present_render_pass == VK_NULL_HANDLE)
                                    {
                                        logger::error("Failed to recreate render pass");
                                                    return;
                                                }
```

整个函数体存在 10+ 级不一致缩进，是格式化工具多次失控的遗留问题。虽然不影响编译，但严重阻碍代码审查和维护。

#### 4. 硬编码绝对路径（不可移植）

```cpp
// load_mesh() 中：
std::string obj_path = "D:/TechArt/Vulkan/model/三角化网格.obj";

// initialize_materials() 中：
material_loader_->set_base_directory("D:/TechArt/Vulkan/materials/");
material_loader_->set_texture_directory("D:/TechArt/Vulkan/");
```

**影响**：在其他机器或其他路径下编译运行时，模型和材质加载静默失败（`load_mesh` 退化为默认立方体，材质全部失效）。应改为相对路径或通过
`Config`/环境变量配置。

### 🟡 中风险

#### 5. `record_scene_command_buffer()` 复用按帧索引的命令缓冲，但场景渲染在帧外等待

```cpp
auto& cmd = cmd_buffers_[frame_index];  // 按 frame_index 选择命令缓冲
// ...
vkQueueWaitIdle(...);  // 场景渲染已完成
// 接着：
auto& cmd = cmd_buffers_imgui_[image_index];  // ImGui 使用 image_index
```

`frame_index`（当前帧，0 或 1）与 `image_index`（交换链图像索引，可能是 0/1/2）是不同的维度。场景命令缓冲按 `frame_index` 分配，而
ImGui 命令缓冲按 `image_index` 分配，两套索引混用容易在逻辑上引入错误，尤其是在三缓冲模式下。

#### 6. `viewport_render_pass_` 成员变量声明了但从未被赋值和使用

```cpp
// 成员变量：
VkRenderPass viewport_render_pass_ = VK_NULL_HANDLE;  // 声明
// 整个文件中没有任何地方给它赋值或使用它
```

项目实际使用 `render_target_render_pass_`，这个 `viewport_render_pass_` 是遗留的死代码，造成混淆。

#### 7. `cube_pass_` 是一个悬空观察指针（raw pointer to unique_ptr owned object）

```cpp
auto cube_pass = std::make_unique<rendering::CubeRenderPass>(cube_config);
cube_pass_     = cube_pass.get();  // 裸指针观察
render_graph_.builder().add_node(std::move(cube_pass));  // 所有权转移给 render_graph_
```

`cube_pass_` 的生命周期依赖 `render_graph_` 中 node 的存活。若 `render_graph_` 被 `clear()` 或 `compile()` 清空节点，
`cube_pass_` 就成为悬空指针。目前没有任何保护机制。

#### 8. `on_render()` 中 ImGui 信号量使用了 `image_index` 而非 `frame_index`

```cpp
VkSemaphore signal_semaphores[] = {
    frame_sync_->get_render_finished_semaphore(image_index).handle()
};
```

`FrameSyncManager` 按 `image_count` 分配 render_finished semaphore，而 `image_index` 来自交换链 `acquire_next_image()`
，取值范围是 `[0, image_count)`，这里使用 `image_index` 是正确的——但与场景渲染中使用 `frame_index`
形成明显的不对称，需要注释说明为何如此设计，否则容易被误认为 bug。

### 🟢 低风险

#### 9. `create_default_cube()` 使用 `HOST_VISIBLE` 内存，性能差

```cpp
VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
```

默认立方体顶点/索引缓冲使用主机可见内存（CPU 内存），不是 GPU 本地内存（`DEVICE_LOCAL`），渲染性能显著低于正确的 staging
buffer → device-local buffer 上传流程。

#### 10. `update_mvp_matrix()` 中 `aspect_ratio` 来自窗口尺寸而非 Viewport 尺寸（降级路径）

```cpp
float aspect_ratio = static_cast<float>(width_) / static_cast<float>(height_);
if (viewport_) {
    aspect_ratio = viewport_->aspect_ratio();  // 正常路径
}
```

降级路径使用整个窗口的宽高比，而不是 viewport 面板的宽高比，会导致立方体在 viewport 面板比例与窗口不同时出现拉伸变形。

---

## 修复建议

1. **去掉每帧 `vkQueueWaitIdle`**：改用 semaphore 链式同步（scene_render_finished_semaphore → imgui_wait），实现真正的
   CPU/GPU 流水线并行。
2. **删除每帧 `logger::info`**：仅在调试模式或首帧输出，或改用 `logger::debug`（配合日志级别过滤）。
3. **替换硬编码路径**：使用 `std::filesystem::current_path()` 或通过 `ApplicationConfig` 传入资产根目录。
4. **整理格式**：对 `on_window_resize()` 和 `on_render()` 整体运行 `clang-format`。
5. **`cube_pass_` 生命周期保护**：在 `RenderGraph::clear()` 时将 `cube_pass_` 置 null，或从 `render_graph_`
   通过名称查询节点而非持有裸指针。
6. **删除 `viewport_render_pass_` 死代码**。

# 代码审查报告 — SceneViewport

**文件路径**

- `include/rendering/SceneViewport.hpp`
- `src/rendering/SceneViewport.cpp`

**审查日期**: 2026-03-15
**严重程度图例**: 🔴 高风险 | 🟡 中风险 | 🟢 低风险

---

## 功能摘要

`SceneViewport` 是一个**离屏渲染目标**，将 3D 场景渲染到纹理，再通过 ImGui 显示在编辑器面板中。它独立管理自己的 Vulkan
图像、内存、图像视图、RenderPass 和 Framebuffer（使用裸 Vulkan API，无 VMA）。

| 资源          | 类型                                           | 管理方式                        |
|-------------|----------------------------------------------|-----------------------------|
| 颜色图像        | `VkImage` + `VkDeviceMemory` + `VkImageView` | 手动创建/销毁                     |
| 深度图像        | `VkImage` + `VkDeviceMemory` + `VkImageView` | 手动创建/销毁                     |
| RenderPass  | `VkRenderPass`                               | 每次 resize 重建                |
| Framebuffer | `VkFramebuffer`                              | 每次 resize 重建                |
| ImGui 采样器   | `VkSampler`                                  | 延迟初始化，跨 resize 复用           |
| ImGui 描述符集  | `VkDescriptorSet`                            | resize 后标记为 NULL，由 ImGui 重建 |

**Resize 策略**：延迟 resize（`request_resize` 标记 → `apply_pending_resize` 实际执行），避免渲染中途重建资源。

---

## 潜在问题

### 🔴 高风险

#### 1. `cleanup()` 不销毁 `imgui_sampler_`，但析构函数额外销毁——销毁顺序存在双重释放风险

```cpp
void SceneViewport::cleanup() {
    // ...
    imgui_descriptor_set_ = VK_NULL_HANDLE;
    // Note: imgui_sampler_ is not destroyed here to allow reuse during resize
    // imgui_sampler_ 在 cleanup() 中保留！
}

SceneViewport::~SceneViewport() {
    cleanup();
    // Destroy ImGui sampler
    if (imgui_sampler_ != VK_NULL_HANDLE && device_)
        vkDestroySampler(device_->device(), imgui_sampler_, nullptr);
}
```

`resize()` 内部调用 `cleanup()`，此时 `imgui_sampler_` 保留。但 `imgui_texture_id()` 中在 resize 后重新注册了新的
`imgui_descriptor_set_`（通过 `ImGui_ImplVulkan_AddTexture`），旧描述符集并未被 `ImGui_ImplVulkan_RemoveTexture()` 释放，导致
ImGui 描述符池泄漏。

#### 2. `transition_image_layout()` 使用 `vkQueueWaitIdle` 阻塞 GPU，每次 resize 都会触发

```cpp
vkQueueSubmit(device_->graphics_queue(), 1, &submit_info, VK_NULL_HANDLE);
vkQueueWaitIdle(device_->graphics_queue()); // 阻塞等待
```

**影响**：每次 viewport resize 都会完全阻塞图形队列，暂停帧渲染。在编辑器窗口快速拖拽 resize 时，会导致明显卡顿。此处应使用
Fence 或将 layout transition 合并到渲染流程中。

#### 3. `imgui_texture_id()` 是 `const` 成员函数但通过 `const_cast` 修改成员变量

```cpp
ImTextureID SceneViewport::imgui_texture_id() const
{
    if (imgui_sampler_ == VK_NULL_HANDLE)
    {
        vkCreateSampler(..., &const_cast<SceneViewport*>(this)->imgui_sampler_);
    }
    if (imgui_descriptor_set_ == VK_NULL_HANDLE && ...)
    {
        const_cast<SceneViewport*>(this)->imgui_descriptor_set_ = ImGui_ImplVulkan_AddTexture(...);
    }
    return reinterpret_cast<ImTextureID>(imgui_descriptor_set_);
}
```

`const_cast` 掩盖了这两个成员的可变性，违背了 `const` 语义。应将 `imgui_sampler_` 和 `imgui_descriptor_set_` 声明为
`mutable`，或将创建逻辑移至非 const 的初始化函数（如 `initialize()`）。

### 🟡 中风险

#### 4. `resize()` 实现中格式混乱（不一致的缩进）

```cpp
void SceneViewport::resize(uint32_t width, uint32_t height)
{
                    if (width == width_ && height == height_)


                    {
                        display_width_ = width;


                        display_height_ = height;
```

`.cpp` 文件中大量存在极端缩进错误（可能由格式化工具失控导致），严重影响代码可读性，排查问题时容易产生视觉干扰。整个
`resize()`、`request_resize()` 和 `apply_pending_resize()` 方法都受此影响。

#### 5. `create_render_pass()` 每次 resize 都新建 RenderPass，但 pipeline 可能缓存了旧 RenderPass 句柄

```cpp
void SceneViewport::resize(uint32_t width, uint32_t height) {
    cleanup();             // 销毁旧 render_pass_
    create_images();
    create_render_pass();  // 创建新 render_pass_（句柄值改变）
    create_framebuffer();
}
```

`CubeRenderPass`/`Material` 中的管线是使用旧 `render_pass_` 句柄创建的。resize 后 `render_pass()`
返回新句柄，但已创建的管线不会自动重建，导致渲染管线与 RenderPass 不兼容（Vulkan 验证层报错）。

#### 6. `display_width_`/`display_height_` 与 `width_`/`height_` 区分不清楚

```cpp
// request_resize 中立即更新 display 尺寸
display_width_ = width;
display_height_ = height;

// resize() 中最终同步 width_/height_
width_ = width;
height_ = height;
```

"立即重建模式"注释说明 `width_ == display_width_`，但过渡期间两者可能不同。`extent()` 返回渲染目标尺寸，`display_extent()`
返回显示尺寸，调用方需要清楚地知道哪个时机用哪个接口，容易混淆。

### 🟢 低风险

#### 7. `create_render_pass()` 缺少 subpass dependency，可能导致布局转换不同步

```cpp
VkRenderPassCreateInfo render_pass_info = {};
render_pass_info.subpassCount           = 1;
render_pass_info.pSubpasses             = &subpass;
// 没有设置 pDependencies！
```

颜色附件从 `UNDEFINED` 到 `SHADER_READ_ONLY_OPTIMAL` 的转换依赖 render pass 的 subpass dependency 来正确同步后续的
texture sampling。缺少依赖定义，驱动可能在不同平台上产生不一致的同步行为。

#### 8. `initialize()` 函数体格式损坏（随机换行）

```cpp
void SceneViewport::initialize(std::shared_ptr<vulkan::DeviceManager> device, const CreateInfo& info)

{
    device_ = device;

    width_ = info.width;

    height_ = info.height;
```

每行之间都有额外的空行，与其他文件的代码风格不一致，同样是格式化工具失控的遗留问题。

---

## 修复建议

1. **修复 ImGui 描述符集泄漏**：在 `cleanup()` 中调用 `ImGui_ImplVulkan_RemoveTexture(imgui_descriptor_set_)` 释放旧描述符集。
2. **将 `imgui_sampler_` 和 `imgui_descriptor_set_` 标记为 `mutable`**，去除 `const_cast`，或在 `initialize()` 中提前创建采样器。
3. **添加 subpass dependency**：定义 `VK_SUBPASS_EXTERNAL` 依赖，确保 `SHADER_READ_ONLY_OPTIMAL` 转换的正确同步。
4. **运行代码格式化**：对 `.cpp` 文件整体重新运行 `clang-format`，修复缩进问题。
5. **resize 后通知下游**：通过回调或事件系统通知 Pipeline/Material 重建，避免 RenderPass 句柄悬空。

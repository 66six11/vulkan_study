# Phase 3 迁移修复日志

## 问题汇总与修复

### 1. 初始化顺序错误

**问题**: `editor_->initialize()` 在 `initialize_viewport()` 之前调用，导致传递给 Editor 的 `viewport_` 为 null。

**修复**: 调整初始化顺序，确保 Viewport 先创建再传递给 Editor。

```cpp
// main.cpp - 调整后的初始化顺序
initialize_viewport();  // 先创建 Viewport 和 RenderTarget

editor_->initialize(window(), device, swap_chain, render_target_, viewport_);  // 再初始化 Editor
```

### 2. Viewport Resize 后 Framebuffer 未重建

**问题**: 当 ImGui 窗口 resize 时，Viewport 会重建 Image/ImageView，但 Framebuffer 仍在使用旧的 ImageView，导致验证层错误：

```
Validation layer: vkCmdBeginRenderPass(): pCreateInfo->pAttachments[0] VkImageView is invalid.
```

**修复**: 添加 Viewport resize 回调机制，在 Viewport resize 后立即重建 Framebuffer。

**修改文件**:

- `include/editor/Editor.hpp`: 添加 `ViewportResizeCallback` 和 `set_viewport_resize_callback()`
- `include/rendering/Viewport.hpp`: 添加 `is_resize_pending()` 和 `pending_extent()` 方法
- `src/editor/Editor.cpp`: 在 `begin_frame()` 中检测 resize 并调用回调
- `src/main.cpp`: 设置回调函数重建 Framebuffer

```cpp
// Editor.hpp
using ViewportResizeCallback = std::function<void(uint32_t width, uint32_t height)>;
void set_viewport_resize_callback(ViewportResizeCallback callback);

// main.cpp - 设置回调
editor_->set_viewport_resize_callback([this](uint32_t width, uint32_t height) {
    create_render_target_framebuffer();
});
```

### 3. Resize 顺序问题

**问题**: 回调中 Framebuffer 重建时使用了旧的 RenderTarget 尺寸。

**修复**: 调整 resize 顺序，先让 RenderTarget resize，再创建 Framebuffer。

```cpp
// Editor.cpp - begin_frame()
if (viewport_->is_resize_pending())
{
    VkExtent2D new_extent = viewport_->pending_extent();
    
    // 1. 先让 RenderTarget resize（重建 Image/ImageView）
    viewport_->apply_pending_resize();
    
    // 2. 然后创建新的 Framebuffer（使用新的 ImageView）
    if (viewport_resize_callback_)
    {
        viewport_resize_callback_(new_extent.width, new_extent.height);
    }
}
```

## 验证

修复后程序能够：

1. 正确初始化 Viewport 和 RenderTarget
2. 在 ImGui 视口 resize 时正确重建 Framebuffer
3. 无验证层错误
4. 正确显示渲染图像

## 修改文件列表

1. `include/editor/Editor.hpp` - 添加 resize 回调机制
2. `include/rendering/Viewport.hpp` - 添加 pending 状态查询方法
3. `src/editor/Editor.cpp` - 实现 resize 回调触发
4. `src/main.cpp` - 调整初始化顺序，添加回调实现

## 注意事项

- `create_render_target_framebuffer()` 中添加了 `vkDeviceWaitIdle()`，确保 GPU 完成后再销毁旧 Framebuffer
- 使用延迟 resize 机制（`request_resize` + `apply_pending_resize`）避免在 ImGui 渲染过程中修改 Vulkan 资源

# 代码审查报告：Viewport

**文件路径**

- `include/rendering/Viewport.hpp`
- `src/rendering/Viewport.cpp`

---

## 功能概述

`Viewport` 管理编辑器中的 3D 视窗显示：

- **延迟 Resize**：通过 `request_resize()` 标记 pending，在帧开始时 `apply_pending_resize()` 实际执行
- **ImGui 纹理 ID**：通过 `ImGui_ImplVulkan_AddTexture` 将离屏渲染结果包装为 ImGui 可显示的纹理
- **Sampler 管理**：懒创建 `VkSampler`，用于 ImGui 采样
- **Resize 回调**：注册外部回调，在 resize 时通知相机控制器等模块

---

## 关键设计

| 特性        | 说明                                                |
|-----------|---------------------------------------------------|
| 延迟 Resize | 避免在渲染过程中触发资源重建                                    |
| ImGui 集成  | `imgui_texture_id()` 提供 ImGui `Image()` 调用所需的纹理句柄 |
| 回调系统      | `std::function` 回调通知外部 resize 事件                  |

---

## 潜在问题

### 🔴 高风险

1. **`imgui_texture_id()` 中 `const_cast` 修改 mutable 成员**  
   函数声明为 `const`，但通过 `const_cast<Viewport*>(this)` 修改 `imgui_sampler_` 和 `imgui_descriptor_set_`，这是对 const
   语义的显式违背。  
   **建议**：将这两个成员标记为 `mutable`，语义上表明它们是懒初始化缓存。

2. **Resize 后旧 ImGui 描述符集未显式释放**  
   `imgui_descriptor_set_` 通过 `ImGui_ImplVulkan_AddTexture` 创建，resize 后清零该指针但未调用
   `ImGui_ImplVulkan_RemoveTexture()`，旧描述符集可能在 ImGui 内部泄漏（虽然 ImGui 在 shutdown 时会清理，但运行时一直累积）。

### 🟡 中风险

3. **`apply_pending_resize()` 无最小尺寸限制**  
   当用户将视窗拖至极小（如 1x1 像素）时，重建渲染目标不会有问题，但投影矩阵的宽高比接近 0，可能导致渲染异常。  
   **建议**：添加最小尺寸限制（如 8x8）。

4. **`resize_callback_` 未支持多个监听者**  
   单一 `std::function` 只能注册一个回调，若多个模块需要监听 resize 事件（如相机和渲染 pass），只能注册一个。

### 🟢 低风险

5. **`Sampler` 的 mipmap 模式固定**  
   Sampler 的 `mipmapMode` 固定为 `VK_SAMPLER_MIPMAP_MODE_LINEAR`，而 viewport 渲染目标只有 1 个 mip level，性能影响可忽略但语义不准确。

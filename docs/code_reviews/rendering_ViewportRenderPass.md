# 代码审查报告：ViewportRenderPass

**文件路径**

- `include/rendering/render_graph/ViewportRenderPass.hpp`
- `src/rendering/render_graph/ViewportRenderPass.cpp`

---

## 功能概述

`ViewportRenderPass` 是 Render Graph 中的离屏渲染 Pass：

- **子 Pass 容器**：`add_sub_pass()` 添加子渲染 Pass，`execute()` 时依次执行
- **自建 RenderPass**：`create_render_pass()` 基于 `RenderTarget` 的格式创建专用 `VkRenderPass`
- **自建 Framebuffer**：`create_framebuffer()` 基于 `RenderTarget` 的 ImageView 创建 Framebuffer
- **Resize 支持**：`recreate_framebuffer()` 在尺寸变化时重建 Framebuffer

---

## 关键设计

| 特性              | 说明                                               |
|-----------------|--------------------------------------------------|
| 子 Pass 模式       | 聚合多个 `RenderPassBase`，统一管理 begin/end render pass |
| RenderTarget 解耦 | 通过 `shared_ptr<RenderTarget>` 引用渲染目标             |
| 依赖声明            | `get_image_outputs()` 返回空（未与 RenderGraph 资源系统整合） |

---

## 潜在问题

### 🔴 高风险

1. **`cleanup_framebuffer()` 销毁 RenderPass**  
   `cleanup_framebuffer()` 同时销毁了 `render_pass_`，而 `recreate_framebuffer()` 只调用 `cleanup_framebuffer()` +
   `create_framebuffer()`，不重新创建 `render_pass_`，导致 resize 后 `render_pass_` 为 `VK_NULL_HANDLE`，后续 execute
   会崩溃。  
   **建议**：`recreate_framebuffer()` 应同时重建 render_pass，或将 render_pass 和 framebuffer 分离管理。

2. **`cleanup_framebuffer()` 中调用 `vkDeviceWaitIdle`**  
   每次 resize 都会阻塞 GPU，在频繁 resize（如拖拽窗口边框）时会导致明显卡顿。

### 🟡 中风险

3. **`setup()` 中 `(void)builder` 表明资源依赖未声明**  
   未声明任何资源依赖，Render Graph 无法为此 Pass 生成正确的 barrier，降低了 Render Graph 的自动化价值。

4. **子 Pass 的 `execute()` 在 render pass 范围内调用但不能再开启 render pass**  
   子 Pass 的 `execute()` 中若尝试调用 `begin_render_pass()`，会在已开启的 render pass 中嵌套另一个 render pass，这在
   Vulkan 中是非法的。

### 🟢 低风险

5. **`get_image_outputs()` 始终返回空**  
   无法向 Render Graph 声明输出图像，无法与其他 Pass 建立依赖关系。

# 代码审查报告：RenderTarget

**文件路径**

- `include/rendering/resources/RenderTarget.hpp`
- `src/rendering/resources/RenderTarget.cpp`

---

## 功能概述

`RenderTarget` 管理离屏渲染所需的颜色和深度附件：

- **颜色附件**：`VkImage` + `VkImageView` + `VkDeviceMemory`，支持可配置格式
- **深度附件**：可选深度图像
- **Resize**：`cleanup()` + 重建，通过 `vkDeviceWaitIdle` 确保 GPU 空闲
- **ImGui 集成**：`create_imgui_descriptor_set()` 创建 ImGui 采样描述符

---

## 关键设计

| 特性         | 说明                                             |
|------------|------------------------------------------------|
| 独立 Sampler | 为每个 RenderTarget 创建专用 Sampler                  |
| 可选深度附件     | `Config::has_depth` 控制是否创建深度图                  |
| 格式查询       | 深度格式通过 `DepthBuffer::find_depth_format()` 动态选择 |

---

## 潜在问题

### 🔴 高风险

1. **`cleanup()` 中调用 `vkDeviceWaitIdle` 阻塞渲染**  
   每次 resize 触发 `cleanup()` 时会等待所有 GPU 命令完成，在编辑器中拖拽视窗边缘时会导致明显卡顿。  
   **建议**：使用延迟销毁队列（deferred deletion），在确认旧资源不再使用后再释放。

2. **`transition_image_layout()` 每次 resize 都创建/销毁临时命令池**  
   布局转换需要创建临时命令池和命令缓冲，resize 频繁时效率低下。  
   **建议**：使用共享的临时命令池，或将布局转换任务排入已有命令缓冲。

### 🟡 中风险

3. **`create_imgui_descriptor_set()` 依赖 ImGui 后端已初始化**  
   若在 `ImGuiManager::initialize()` 之前调用，`ImGui_ImplVulkan_AddTexture` 会失败（ImGui 内部池未准备好）。代码中没有初始化顺序的文档或断言。

4. **Sampler 被硬编码为线性过滤**  
   `VK_FILTER_LINEAR` 对于某些调试需求（如查看法线图、ID 缓冲）不合适，应支持可配置。

### 🟢 低风险

5. **`has_color()` / `has_depth()` 基于 ImageView 是否非空推断**  
   状态判断依赖 Vulkan 句柄是否为 `VK_NULL_HANDLE`，而非配置标志，若中间状态下句柄被意外清零，会误判渲染目标配置。

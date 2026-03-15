# 代码审查报告：SwapChain

**文件路径**

- `include/vulkan/device/SwapChain.hpp`
- `src/vulkan/device/SwapChain.cpp`

---

## 功能概述

`SwapChain` 管理 Vulkan 交换链完整生命周期：

- **创建**：选择最优 Surface Format、Present Mode、Extent
- **ImageView 创建**：为每个 Swapchain 图像创建 `VkImageView`
- **RenderPass 创建**：`create_default_render_pass()` 和 `create_render_pass_with_depth()`
- **Framebuffer 创建**：为每个图像创建对应 Framebuffer
- **`recreate()`**：响应窗口 resize，重建所有交换链相关资源
- **帧获取**：`acquire_next_image()` / `present()` 驱动帧渲染

---

## 关键设计

| 特性           | 说明                                                   |
|--------------|------------------------------------------------------|
| RAII         | 析构时按序销毁 Framebuffers、RenderPass、ImageViews、SwapChain |
| Format 选择    | 优先 `B8G8R8A8_SRGB` + `NONLINEAR_KHR`                 |
| Present Mode | 优先 Mailbox（三缓冲），次选 FIFO                              |

---

## 潜在问题

### 🔴 高风险

1. **`shutdown()` 中格式异常代码**  
   代码中存在 `device_&& device_\n\n->\ndevice()` 格式化工具损坏的代码片段，可能导致编译错误或逻辑断裂。  
   **建议**：立即检查并修复该处代码。

2. **`graphics_queue_family_` 硬编码为 0**  
   SwapChain 创建时假设 Graphics 队列族 index 为 0，未从 `DeviceManager` 获取实际值，在部分 GPU 上会创建失败。

### 🟡 中风险

3. **与 `RenderPassManager` 功能重叠**  
   `create_default_render_pass()` 和 `create_render_pass_with_depth()` 与 `RenderPassManager` 中的 RenderPass
   创建逻辑重复，维护两套实现增加了同步成本。  
   **建议**：统一通过 `RenderPassManager` 管理所有 RenderPass。

4. **`recreate()` 中未等待所有命令完成**  
   recreate 前仅调用 `vkDeviceWaitIdle`，在极端情况下（如多线程提交）可能存在资源竞争。

### 🟢 低风险

5. **Present Mode 选择策略固定**  
   Mailbox → FIFO 的选择策略固定，无法根据用户配置（V-Sync 开关）动态调整。

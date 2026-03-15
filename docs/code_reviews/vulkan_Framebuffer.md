# 代码审查报告：Framebuffer

**文件路径**

- `include/vulkan/resources/Framebuffer.hpp`
- `src/vulkan/resources/Framebuffer.cpp`

---

## 功能概述

`Framebuffer` 封装 `VkFramebuffer` RAII 对象：

- **双构造重载**：支持从 `FramebufferConfig` 结构体或显式参数创建
- **`FramebufferBuilder`**：链式 API 配置 attachment、dimensions、layers
- **`FramebufferPool`**：批量管理交换链 Framebuffer 集合，支持深度附件可选附加

---

## 关键设计

| 特性              | 说明                       |
|-----------------|--------------------------|
| RAII            | 析构时自动销毁 `VkFramebuffer`  |
| 完整移动语义          | 移动后原对象 `framebuffer_` 清零 |
| FramebufferPool | 专门针对交换链多图像场景的批量管理        |

---

## 潜在问题

### 🟡 中风险

1. **`FramebufferPool` 不验证 RenderPass 兼容性**  
   `create_for_swap_chain()` 时不检查传入的 `render_pass` 与 attachment 格式是否匹配，不兼容时会在运行时触发 Vulkan
   验证错误，而非在创建时给出明确提示。

2. **`FramebufferAttachment::format` 字段未被使用**  
   `FramebufferAttachment` 结构体包含 `format` 字段，但 `Framebuffer` 创建时只提取 `image_view`，`format`
   信息被忽略，这使得结构体携带冗余字段。

3. **`get_framebuffer(index)` 越界行为静默**  
   当 `index >= framebuffers_.size()` 时返回 `nullptr`，调用者若未检查 null 指针，会在后续 Vulkan 操作中解引用 null。  
   **建议**：添加断言或抛出异常，使越界行为明显。

### 🟢 低风险

4. **`FramebufferBuilder::build()` 未验证必要字段**  
   若未调用 `render_pass()` 设置 RenderPass，`build()` 会创建带 `VK_NULL_HANDLE` render_pass 的 Framebuffer，导致 Vulkan
   创建失败。

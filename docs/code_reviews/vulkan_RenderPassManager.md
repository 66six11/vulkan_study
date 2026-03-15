# 代码审查报告：RenderPassManager

**文件路径**

- `include/vulkan/pipelines/RenderPassManager.hpp`
- `src/vulkan/pipelines/RenderPassManager.cpp`

---

## 功能概述

`RenderPassManager` 集中管理 `VkRenderPass` 对象：

- **哈希缓存**：通过 `RenderPassKey`（attachment 描述符哈希）避免重复创建
- **预设类型**：支持 present / offscreen / depth-only / shadow 四种常用 RenderPass
- **生命周期**：`cleanup_unused()` 清理缓存，析构时销毁全部

---

## 关键设计

| 特性   | 说明                                       |
|------|------------------------------------------|
| 哈希缓存 | `RenderPassKey` 哈希附件描述，相同配置复用            |
| 工厂方法 | `create_present_render_pass()` 等静态工厂简化创建 |
| 线程感知 | 所有操作都需要调用方保证单线程（无锁）                      |

---

## 潜在问题

### 🔴 高风险

1. **`cleanup_unused()` 实际清空所有缓存**  
   `cleanup_unused()` 方法无引用计数，直接调用 `clear()` 销毁全部 RenderPass，正在被 Pipeline 引用的 RenderPass
   会因此变成悬空句柄，导致渲染崩溃。  
   **建议**：引入引用计数（`use_count`），仅清理引用计数为 0 的 RenderPass。

### 🟡 中风险

2. **`clear()` 中调用 `vkDeviceWaitIdle`**  
   在频繁创建/销毁场景下（如编辑器频繁修改材质），每次 `clear()` 都会等待 GPU 空闲，造成明显的帧率抖动。

3. **与 SwapChain 中 RenderPass 创建逻辑重叠**  
   `SwapChain` 中也有独立的 `create_default_render_pass()` 实现，与 `RenderPassManager` 重复，维护两套实现存在分歧风险。

### 🟢 低风险

4. **哈希冲突处理**  
   `RenderPassKey` 的哈希函数基于简单位运算（XOR/shift），对复杂附件组合可能产生哈希冲突，应验证其碰撞概率。

# 代码审查报告：RenderGraphResource

**文件路径**

- `include/rendering/render_graph/RenderGraphResource.hpp`
- `src/rendering/render_graph/RenderGraphResource.cpp`

---

## 功能概述

Render Graph 资源管理层：

- **`RenderGraphResourcePool`**：按 handle ID 管理 Image/Buffer 资源，支持资源创建、导入外部资源（如 SwapChain 图像）、Barrier
  生成
- **`BarrierManager`**：跨 Pass 分析资源状态转换，编译为 `BarrierBatch`
- **`ResourceState`**：追踪每个资源的当前 pipeline stage、access mask、image layout

---

## 关键设计

| 特性         | 说明                                        |
|------------|-------------------------------------------|
| Handle 系统  | `ImageHandle` / `BufferHandle` 类型安全句柄     |
| 外部资源导入     | `import_image()` 支持 SwapChain 图像等外部资源     |
| 自动 Barrier | `generate_barriers()` 仅在状态真正变化时生成 barrier |

---

## 潜在问题

### 🔴 高风险

1. **外部资源（`is_external = true`）的 barrier 被跳过**  
   `generate_barriers()` 中对外部资源（如 SwapChain 图像）直接返回，不生成任何 barrier。这意味着对 SwapChain 图像的布局转换（如
   `PRESENT_SRC` → `COLOR_ATTACHMENT`）不会被自动处理，需要调用方手动管理，与 Render Graph 的自动化理念矛盾。

2. **`BarrierManager::compile()` 按顺序推导状态不正确**  
   `compile()` 基于 pass 索引顺序（0, 1, 2...）查找前一个 pass 的状态，但 pass 资源注册用的 key 是 pass index，若
   `pass_resources_` 不是从 0 连续的 map，迭代行为将不符合预期。

### 🟡 中风险

3. **`acquire_image()` 不支持资源版本化（generation 未校验）**  
   `ResourceHandle` 包含 `generation` 字段，但 `acquire_image()` 仅用 `handle.id()` 作为 key 查找，`generation` 完全被忽略，过期的
   handle 会返回未过期的资源。

4. **`create_buffer()` 资源描述的 format 字段被忽略**  
   `BufferResourceInfo` 的 `desc` 中有 `format` 字段，但 `create_buffer()` 只使用 `desc.size` 和 `desc.type`。

### 🟢 低风险

5. **`reset()` 不清理 Buffer 的 view**  
   `reset()` 中遍历 image_barriers 并销毁 image view，但没有对 buffer 做类似清理（Buffer 无 view，可接受）。

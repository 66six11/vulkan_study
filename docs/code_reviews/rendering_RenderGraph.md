# 代码审查报告：RenderGraph

**文件路径**

- `include/rendering/render_graph/RenderGraph.hpp`
- `src/rendering/render_graph/RenderGraph.cpp`

---

## 功能概述

`RenderGraph` 是声明式渲染框架的核心：

- **`RenderGraphNode`**：渲染图节点基类，声明 buffer/image 输入输出
- **`RenderGraph`**：管理节点列表，`compile()` 生成执行顺序，`execute()` 驱动渲染
- **`RenderGraphBuilder`**：Pass 设置阶段的资源声明接口（`readTexture`, `writeTexture` 等）
- **`CommandBuffer` 抽象**：简单的 `CommandBuffer` 基类，供图节点的 execute 使用

---

## 关键设计

| 特性           | 说明                                      |
|--------------|-----------------------------------------|
| 声明式设计        | Pass 在 setup 阶段声明依赖，Graph 自动推导执行顺序      |
| 节点注册         | `add_node()` 将 Pass 加入图，`compile()` 前调用 |
| Barrier 自动生成 | `generate_barriers()` 根据节点依赖生成同步屏障      |

---

## 潜在问题

### 🔴 高风险

1. **`build_execution_order()` 无拓扑排序**  
   当前实现直接按添加顺序作为执行顺序，没有进行真正的拓扑排序。当 Pass
   之间存在非线性依赖时，执行顺序可能错误，导致使用未更新的资源。  
   **建议**：实现 Kahn 算法或 DFS 拓扑排序。

2. **`generate_barriers()` 依赖 `dynamic_cast<RenderPassBase*>`**  
   非 `RenderPassBase` 派生的节点无法生成 barriers，会跳过同步屏障，导致 GPU 读写竞争。

### 🟡 中风险

3. **`RenderGraphBuilder::next_id` 为 static 局部变量**  
   多个 `RenderGraphBuilder` 实例共享同一个 `next_id`，导致资源 ID 全局递增而非每个 Graph 独立，在存在多个 RenderGraph
   实例时会产生 ID 冲突。  
   **建议**：改为成员变量，或传入外部 ID 分配器。

4. **`execute()` 中命令缓冲获取方式不统一**  
   `execute()` 使用简单的 `CommandBuffer` 基类，而实际 `RenderPassBase` 的 `execute()` 需要 `RenderCommandBuffer`
   ，类型系统存在不匹配（通过 `dynamic_cast` 桥接）。

### 🟢 低风险

5. **没有并行执行支持**  
   所有 Pass 串行执行，无法利用 Vulkan 多队列并行提交能力。

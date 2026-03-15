# 代码审查报告：RenderGraphPass

**文件路径**

- `include/rendering/render_graph/RenderGraphPass.hpp`
- `src/rendering/render_graph/RenderGraphPass.cpp`

---

## 功能概述

定义 Render Graph 中的具体 Pass 类型：

- **`RenderPassBase`**：所有 Pass 的基类，实现 `RenderGraphNode` 接口，桥接两套 execute 签名
- **`ClearRenderPass`**：执行颜色/深度清除的 Pass
- **`GeometryRenderPass`**：绘制网格的几何 Pass，支持多 Mesh Draw Call 批处理
- **`PresentRenderPass`**：交换链图像 present 准备 Pass（目前为空实现）
- **`UIRenderPass`**：UI 渲染 Pass（占位符）

---

## 关键设计

| 特性         | 说明                                                   |
|------------|------------------------------------------------------|
| 双接口桥接      | `RenderPassBase` 的 `execute(CommandBuffer&)` 向上适配旧接口 |
| 几何批处理      | `GeometryRenderPass` 支持多 `MeshDraw` 批量绘制             |
| Config 结构体 | 每个 Pass 使用 Config 结构体聚合参数                            |

---

## 潜在问题

### 🔴 高风险

1. **`PresentRenderPass::execute()` 为空实现**  
   Present Pass 完全没有实现（连 barrier 也没有），如果集成到 Render Graph 后预期负责 present 转换，实际上什么都不做，会导致
   Present 格式不正确。

### 🟡 中风险

2. **`GeometryRenderPass` 无 render pass begin/end**  
   `GeometryRenderPass::execute()` 直接绑定管线并绘制，没有 `begin_render_pass` / `end_render_pass`，这要求调用方在此之前已经开启
   render pass，隐式依赖脆弱。

3. **`UIRenderPass` 完全未实现**  
   `execute()` 仅打印一条 debug 日志，在 Render Graph 中使用此 Pass 时不会产生任何 UI 输出，但不会报错，容易混淆。

4. **`GeometryRenderPass` Viewport 尺寸计算**  
   Viewport 宽高判断逻辑（`<= 1.0 ? 相对比例 : 绝对值`）处理方式不明确，当 `viewport_width == 1.0f` 时既可以是"全宽"
   也可以是"1像素宽"，边界条件不清晰。

### 🟢 低风险

5. **`RenderPassBase::execute(CommandBuffer&)` 打印警告**  
   基类的旧接口版本直接打印 warn 日志，在正常调用流程中若有误触发会产生噪声日志。

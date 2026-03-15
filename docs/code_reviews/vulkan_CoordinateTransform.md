# 代码审查报告：CoordinateTransform

**文件路径**

- `include/vulkan/utils/CoordinateTransform.hpp`

---

## 功能概述

`CoordinateTransform` 提供 Vulkan 坐标系转换工具（全静态方法，纯头文件）：

- **`opengl_to_vulkan_projection()`**：翻转投影矩阵 Y 轴（`[1][1] *= -1`），适配 Vulkan NDC
- **`ortho()`**：创建 Vulkan 兼容正交投影矩阵
- **`perspective()`**：创建 Vulkan 兼容透视投影矩阵（接受度数，内部转弧度）
- **`get_y_flip_matrix()`**：返回 Y 轴翻转矩阵，用于手动处理

---

## 关键设计

| 特性     | 说明                                          |
|--------|---------------------------------------------|
| 全静态方法  | 工具类，无状态                                     |
| GLM 集成 | 基于 glm::perspective/ortho 构建，修正 Vulkan 坐标差异 |
| 简洁实现   | 所有方法内联，零运行时开销                               |

---

## 潜在问题

### 🟡 中风险

1. **`fov_y_degrees` 参数单位在接口层不够明确**  
   `perspective()` 接受度数但参数名为 `fov_y_degrees`，虽然名称足够清晰，但如果与 GLM 混用时可能导致混淆（GLM
   `glm::perspective` 接受弧度）。  
   **建议**：在函数注释中显式说明单位为度数。

2. **`opengl_to_vulkan_projection()` 修改矩阵的 `[1][1]` 而非创建新矩阵**  
   直接修改传入矩阵的 Y 轴分量是正确的 Vulkan 适配方式，但对不了解此背景的开发者不直观，应添加注释解释原理。

### 🟢 低风险

3. **未处理右手坐标系 vs 左手坐标系的情况**  
   GLM 默认右手坐标系，Vulkan 使用右手坐标系但 Y 轴向下，当前实现通过 Y 翻转处理，但对于某些使用 `GLM_FORCE_LEFT_HANDED`
   宏的项目配置，此翻转可能不正确。

4. **缺少 Z 深度映射处理**  
   GLM 默认 NDC 深度为 [-1, 1]，Vulkan 为 [0, 1]，当前代码未处理此差异（需要 `GLM_FORCE_DEPTH_ZERO_TO_ONE` 宏或手动变换）。  
   **建议**：在文档中明确说明需要定义 `GLM_FORCE_DEPTH_ZERO_TO_ONE`。

# 代码审查报告：Camera

**文件路径**

- `include/core/math/Camera.hpp`

---

## 功能概述

`OrbitCamera` 实现球坐标系轨道相机，提供：

- **球坐标系**：通过 `theta`（水平角）、`phi`（垂直角）、`radius`（距离）描述相机位置
- **视图矩阵**：`get_view_matrix()` 计算 `glm::lookAt`
- **投影矩阵**：`get_projection_matrix()` 生成 Vulkan 兼容的透视投影（Y 轴翻转）
- **交互接口**：`on_mouse_drag()` / `on_mouse_scroll()` 响应鼠标输入
- **纯头文件实现**：所有方法 `inline` 定义

---

## 关键设计

| 特性          | 说明                                     |
|-------------|----------------------------------------|
| 球坐标系        | 直观的轨道旋转，自然避免 gimbal lock（限制 phi 范围）    |
| Vulkan Y 翻转 | 投影矩阵 `[1][1] *= -1` 修正 Vulkan NDC 坐标系  |
| 参数限制        | `phi` 限制在 `[1°, 179°]`，`radius` 有最小值保护 |

---

## 潜在问题

### 🟡 中风险

1. **Up Vector 硬编码为 `(0, 1, 0)`**  
   当 `phi` 接近 0° 或 180° 时，相机前向量与 up vector 近似平行，`glm::lookAt` 会产生数值不稳定（虽然通过限制 phi 缓解，但
   1° 限制较粗糙）。  
   **建议**：动态计算 up vector，或在 phi 接近极点时切换到 `(0, 0, 1)`。

2. **`on_mouse_drag` 参数顺序不直观**  
   `on_mouse_drag(float dx, float dy, float sensitivity)` 中，dx 对应 theta 旋转（水平），dy 对应 phi 旋转（垂直），参数顺序与直觉一致，但
   sensitivity 作为第三参数可能被误用为 dy。  
   **建议**：使用具名参数结构体或 getter/setter 设置灵敏度。

### 🟢 低风险

3. **纯头文件实现影响编译时间**  
   所有相机逻辑内联在头文件中，在大型项目中会增加编译时间（每个包含此头文件的翻译单元都会实例化这些 inline 函数）。

4. **`fov_degrees` 字段不统一**  
   构造函数接受 `fov_degrees`（度数），投影矩阵内部调用 `glm::radians()` 转换，但没有注释说明单位，容易引起混淆。

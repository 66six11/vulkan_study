# 代码审查报告：InputManager

**文件路径**

- `include/platform/input/InputManager.hpp`
- `src/platform/input/InputManager.cpp`

---

## 功能概述

`InputManager` 管理键盘和鼠标输入状态，采用 **Pimpl + 双缓冲** 设计：

- **键盘状态**：`is_key_pressed`, `is_key_just_pressed`, `is_key_just_released`（帧差分）
- **鼠标状态**：`is_mouse_button_pressed`, `mouse_position`, `mouse_delta`, `scroll_delta`
- **GLFW 回调**：通过 `glfwSetKeyCallback` / `glfwSetCursorPosCallback` 等注册
- **键值映射**：`map_glfw_key()` 将 GLFW 键码映射到引擎 `Key` 枚举
- **`update()`**：每帧调用，更新双缓冲状态，计算 delta

---

## 关键设计

| 特性    | 说明                                                    |
|-------|-------------------------------------------------------|
| 双缓冲   | `current_frame_` / `previous_frame_` 分离，支持"刚按下/刚释放"查询 |
| Pimpl | GLFW 依赖完全封装在 `.cpp` 中                                 |
| 查表映射  | `update()` 内用静态数组映射 GLFW 键码，效率高                       |

---

## 潜在问题

### 🔴 高风险

1. **双重映射逻辑不一致**  
   `map_glfw_key()`（回调路径）只映射了 Space、Escape、W/A/S/D 等 6 个键，而 `update()`（轮询路径）使用独立的 `key_map`
   数组映射更多键。两套路径映射范围不同，可能导致部分键在回调模式下不可用。  
   **建议**：统一为单一映射表，供两条路径共用。

### 🟡 中风险

2. **`map_glfw_key` 对大多数键返回 `Unknown`**  
   该函数目前只处理 6 个键值，其余所有键均返回 `Key::Unknown`，导致回调路径下大量按键无法识别。

3. **`scroll_delta` 每帧累积未清零**  
   若 `update()` 未在每帧正确重置 `scroll_delta_`，滚轮值会跨帧累积，导致不期望的行为。

### 🟢 低风险

4. **`mouse_position` 返回 `double` 精度**  
   GLFW 提供 `double` 精度鼠标坐标，但 `OrbitCameraController` 将其转换为 `float`，在超高分辨率下（>16k）可能损失精度。

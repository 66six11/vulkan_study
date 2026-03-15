# 代码审查报告 — CameraController

**文件路径**

- `include/rendering/camera/CameraController.hpp`
- `src/rendering/camera/CameraController.cpp`

**审查日期**: 2026-03-15
**严重程度图例**: 🔴 高风险 | 🟡 中风险 | 🟢 低风险

---

## 功能摘要

本模块提供相机控制器的抽象基类和具体的轨道相机（Orbit）控制器实现，解耦输入处理与相机逻辑。

| 类                       | 职责                                                        |
|-------------------------|-----------------------------------------------------------|
| `CameraController`      | 抽象基类，管理 camera / input_manager / viewport 的 `weak_ptr` 引用 |
| `OrbitCameraController` | 处理鼠标拖拽旋转和滚轮缩放，支持 ImGui 输入或 InputManager 两路输入              |

**输入双路模式**：

- `use_imgui_input = true`：通过 `ImGui::GetIO()` 获取鼠标位置和滚轮
- `use_imgui_input = false`：通过 `InputManager` 的 `mouse_position()` / `mouse_delta()` / `scroll_delta()`

---

## 设计亮点

- 使用 `weak_ptr` 持有 Camera / InputManager / Viewport，避免循环引用
- `set_enabled(false)` 时自动重置 `is_dragging_` 状态，避免重新启用时出现跳跃
- 注释清晰地记录了 ImGui 输入控制路由的设计意图

---

## 潜在问题

### 🔴 高风险

#### 1. `use_imgui_input = true` 模式下，鼠标 delta 计算与 `use_imgui_input = false` 模式不一致

```cpp
// ImGui 模式：使用 io.MousePos 计算 delta（自己维护 last_mouse_x_/y_）
float dx = current_x - last_mouse_x_;

// InputManager 模式：直接使用 input->mouse_delta()（由 InputManager 计算）
auto [dx, dy] = input->mouse_delta();
```

ImGui 模式下，`delta_x = io.MouseDelta.x` 已被提前读取但在拖拽模式（`require_mouse_drag = true`）中被**完全忽略**，改为手动计算
`current_x - last_mouse_x_`。而悬停模式（`require_mouse_drag = false`）则使用 `io.MouseDelta`。两条路径语义不统一，拖拽模式下如果
ImGui 的 `MouseDelta` 与手算 delta 因帧率差异略有不同，可能造成轻微抖动。

#### 2. `attach_camera()` 只支持 `core::OrbitCamera`，基类接口设计过窄

```cpp
void attach_camera(std::shared_ptr<core::OrbitCamera> camera) { camera_ = camera; }
```

`CameraController` 基类硬绑定了 `core::OrbitCamera` 类型，无法附加其他相机类型（如第一人称相机、飞行相机）。如果未来新增
`CameraController` 子类用于其他相机，基类接口需要修改。应考虑泛化为 `core::CameraBase` 或类似接口。

### 🟡 中风险

#### 3. `camera_.lock()` 失败时静默跳过（无日志警告）

```cpp
void OrbitCameraController::handle_rotation() {
    auto camera = camera_.lock();
    if (!camera) return;  // 静默返回，无任何提示
```

当 `camera_` 所指对象已被销毁（例如 `EditorApplication` 的析构顺序问题），控制器会静默停止工作，没有任何警告。调试时难以发现相机断连的根本原因。

#### 4. `update()` 中 `delta_time` 参数完全未使用

```cpp
void OrbitCameraController::update(float delta_time) {
    (void)delta_time;  // 帧时间被完全忽略
```

旋转速度和缩放速度与帧率直接相关——帧率高时相机移动更快，帧率低时更慢。应将灵敏度参数乘以 `delta_time` 实现帧率无关的相机控制。

#### 5. `handle_rotation()` 在悬停模式下重复读取 `delta_x`/`delta_y` 变量

```cpp
float delta_x = 0.0f, delta_y = 0.0f;
// ... 在 if(config_.use_imgui_input) 分支中设置 delta_x/delta_y
// ... 在 else 分支中设置 delta_x/delta_y

if (config_.require_mouse_drag) {
    // 拖拽模式：delta_x/delta_y 被读取了（ImGui 分支）但实际不使用
    float dx = current_x - last_mouse_x_;  // 自己重新算
} else {
    // 悬停模式：使用 delta_x/delta_y
    camera->on_mouse_drag(delta_x, delta_y, ...);
}
```

变量 `delta_x`/`delta_y` 在拖拽模式的 ImGui 路径中被赋值但从不使用，代码逻辑混乱，可读性差。

### 🟢 低风险

#### 6. `Config` 中 `use_imgui_input` 默认值为 `true`，但 `main.cpp` 覆盖为 `false`

```cpp
// Config 默认值：
bool use_imgui_input = true;

// main.cpp 中覆盖：
controller_config.use_imgui_input = false;
```

这导致 `Config` 的默认值与实际项目使用不一致。如果有人直接用默认 `Config` 创建控制器，会使用 ImGui 输入路径，而
`input_manager_` 的 `weak_ptr` 会是空的（因为没有附加），悬停模式下直接 `return`，行为静默不正确。

#### 7. `attach_viewport()` 接口存在但 `handle_rotation()`/`handle_zoom()` 从不使用 `viewport_`

```cpp
void attach_viewport(std::shared_ptr<Viewport> viewport) { viewport_ = viewport; }
```

`viewport_` 在两个处理函数中都没有被 `lock()` 使用，绑定它没有任何效果。注释说"用于获取宽高比"但实际代码中宽高比从未被读取用于相机控制。

---

## 修复建议

1. **帧率无关性**：将 `rotation_sensitivity` 和 `zoom_speed` 的应用乘以 `delta_time`，避免帧率影响控制手感。
2. **泛化 `attach_camera`**：引入 `CameraBase` 抽象类，`OrbitCamera` 继承它，基类持有 `weak_ptr<CameraBase>`。
3. **统一 delta 计算**：拖拽模式和悬停模式都统一使用 `io.MouseDelta`（ImGui 路径）或 `input->mouse_delta()`（InputManager
   路径），消除重复逻辑。
4. **添加 `weak_ptr` 失效警告**：在 `lock()` 失败时打印一次 `logger::warn`，帮助调试。

# 架构审查报告 - 视窗输入系统修复

**日期**: 2026-03-15  
**版本**: v1.0  
**状态**: 已修复核心问题，待重构优化

---

## 1. 修复摘要

本次修复解决了 Editor 视窗中的鼠标输入问题：

- ✅ **鼠标拖拽旋转**: 已修复
- ✅ **滚轮缩放**: 已修复

---

## 2. 问题根因分析

### 2.1 GLFW User Pointer 冲突

**问题**: `Window` 和 `InputManager` 都调用 `glfwSetWindowUserPointer()`，后者覆盖了前者的设置。

```cpp
// Window 构造函数
GLFWwindow* glfw_window = ...;
glfwSetWindowUserPointer(glfw_window, this);  // Window 指针

// InputManager::initialize()
glfwSetWindowUserPointer(glfw_window, this);  // 覆盖了 Window 指针！
```

**影响**: 所有依赖 `glfwGetWindowUserPointer` 的回调都无法正确获取对象。

**修复**: 统一由 `Window` 管理所有 GLFW 回调，`InputManager` 通过 `Window` 的接口注册回调。

### 2.2 相机控制器未正确附加相机

**问题**: 使用栈上的 `OrbitCamera` 对象创建 `shared_ptr`，导致 `lock()` 失败。

```cpp
// 错误用法
OrbitCamera camera_;  // 栈对象
camera_controller_->attach_camera(
    std::shared_ptr<OrbitCamera>(&camera_, [](auto*) {}));  // 危险！
```

**修复**: 使用 `std::make_shared` 创建相机对象。

### 2.3 事件处理时序问题

**问题**: `InputManager::update()` 在 `on_update()` 之前调用，重置了滚轮值。

```cpp
// 原始顺序 (错误)
input_manager_->update();  // scroll_delta = 0
on_update(delta_time);     // 读取到 0
```

**修复**:

1. 滚轮值改为累积模式，读取后才重置
2. 调整调用顺序：`poll_events` → `update` → `on_update`

### 2.4 ImGui 绘制顺序错误

**问题**: `InvisibleButton` 在 `Image` 之前绘制，导致输入被图像覆盖。

```cpp
// 错误顺序
ImGui::InvisibleButton(...);  // 先绘制
ImGui::SetCursorScreenPos(...);
ImGui::Image(...);            // 覆盖按钮
```

**修复**: 调整绘制顺序：`Image` → 重置光标 → `InvisibleButton`

---

## 3. 架构问题

### 3.1 当前架构 (存在耦合)

```
┌─────────────────────────────────────────────────────────────┐
│                        Application                           │
│  ┌─────────────────┐    ┌───────────────────────────────┐   │
│  │   InputManager  │◄───│      CameraController         │   │
│  │  (GLFW events)  │    │  ┌─────────────────────────┐  │   │
│  └────────┬────────┘    │  │  std::weak_ptr<Camera>  │  │   │
│           │             │  └─────────────────────────┘  │   │
│           │             └───────────────────────────────┘   │
│           │                            ▲                    │
│           │                            │                    │
│  ┌────────▼────────┐                   │                    │
│  │      Window     │───────────────────┘                    │
│  │ (GLFW wrapper)  │  (InputManager 通过 Window 获取回调)   │
│  └─────────────────┘                                       │
└─────────────────────────────────────────────────────────────┘
```

### 3.2 发现的问题

| # | 问题                                      | 严重程度 | 建议                            |
|---|-----------------------------------------|------|-------------------------------|
| 1 | `Window` 和 `InputManager` 耦合过紧          | 🔴 高 | 引入事件总线或消息队列解耦                 |
| 2 | `CameraController` 使用 `weak_ptr` 但缺乏空检查 | 🟡 中 | 添加断言或错误处理                     |
| 3 | `InputManager::scroll_delta()` 副作用不明显   | 🟡 中 | 重命名为 `consume_scroll_delta()` |
| 4 | 视窗输入检测逻辑分散在多个类中                         | 🟡 中 | 集中到 `Viewport` 类              |
| 5 | `InputManager` 状态更新模式不一致                | 🟢 低 | 统一为累积模式或即时模式                  |

---

## 4. 重构建议

### 4.1 短期 (1-2 周)

#### 4.1.1 解耦 Window 和 InputManager

```cpp
// 建议: 使用事件总线
class EventBus {
public:
    void publish(const Event& event);
    void subscribe(EventType type, Callback callback);
};

// Window 只负责发布事件
void Window::poll_events() {
    glfwPollEvents();
    event_bus_->publish(MouseMoveEvent{x, y});
}

// InputManager 订阅事件
void InputManager::initialize(EventBus* bus) {
    bus->subscribe(EventType::MouseMove, [this](auto e) {
        handle_mouse_move(e);
    });
}
```

#### 4.1.2 明确 scroll_delta 的副作用

```cpp
// 当前
double scroll_delta();  // 非 const，会重置值

// 建议
double consume_scroll_delta();  // 名称明确表示会消费值
std::optional<double> try_consume_scroll_delta();  // 或返回 optional
```

### 4.2 中期 (1-2 月)

#### 4.2.1 重构视窗输入系统

当前逻辑分散在：

- `ImGuiManager::draw_editor_layout()` - 检测悬停
- `Editor::is_viewport_content_hovered()` - 转发状态
- `main.cpp` - 启用/禁用控制器

建议集中到 `Viewport` 类：

```cpp
class Viewport {
public:
    bool is_focused() const;
    bool is_hovered() const;
    bool is_content_hovered() const;
    
    // 统一处理输入
    void process_input(InputManager* input);
};
```

#### 4.2.2 相机控制器接口优化

```cpp
// 当前: 使用 weak_ptr
void attach_camera(std::shared_ptr<Camera> camera);

// 建议: 使用原始指针 + 生命周期管理
void set_camera(Camera* camera);
void clear_camera();  // 明确清除

// 或使用观察者模式
camera->add_observer(controller);
```

### 4.3 长期 (3+ 月)

#### 4.3.1 输入系统整体重构

- 支持输入映射配置 (Input Mapping)
- 支持动作/轴抽象 (Action/Axis)
- 支持上下文切换 (UI模式/游戏模式)

```cpp
class InputContext {
public:
    void bind_action("rotate", MouseButton::Left);
    void bind_axis("zoom", ScrollAxis());
};
```

#### 4.3.2 引入 ECS 架构

相机控制可以作为组件系统的一部分：

```cpp
struct CameraComponent {
    OrbitCamera camera;
};

struct CameraControlComponent {
    float sensitivity;
    bool require_drag;
};

// System 处理输入
class CameraControlSystem : public System {
    void update(EntityManager& em, InputManager* input);
};
```

---

## 5. 代码质量改进

### 5.1 当前代码异味

| 文件                     | 问题          | 建议              |
|------------------------|-------------|-----------------|
| `CameraController.cpp` | 嵌套条件过深      | 提取函数            |
| `InputManager.cpp`     | 使用了原始数组     | 改用 `std::array` |
| `ImGuiManager.cpp`     | 绘制逻辑和输入检测耦合 | 分离为两个函数         |
| `main.cpp`             | 过多初始化逻辑     | 提取到工厂类          |

### 5.2 测试覆盖建议

需要添加的测试：

- `InputManager` 状态转换测试
- `CameraController` 拖拽逻辑测试
- `Viewport` 悬停检测测试
- GLFW 回调集成测试

---

## 6. 相关文件变更

### 6.1 本次修改的文件

| 文件                         | 变更类型 | 说明                          |
|----------------------------|------|-----------------------------|
| `Window.hpp/cpp`           | 新增接口 | 添加输入回调和 `set_input_manager` |
| `InputManager.hpp/cpp`     | 重构   | 移除 GLFW 直接操作，改用 Window 接口   |
| `CameraController.hpp/cpp` | 新增   | 添加 `set_enabled` 虚函数        |
| `ImGuiManager.cpp`         | 修复   | 调整绘制顺序，修复输入检测               |
| `main.cpp`                 | 修复   | 使用 `shared_ptr` 管理相机        |
| `Application.cpp`          | 优化   | 调整事件处理顺序                    |

### 6.2 需要后续关注的文件

- `SceneViewport.cpp` - 考虑添加输入处理接口
- `RenderGraph.cpp` - 与视窗系统的交互
- `Editor.cpp` - 协调 ImGui 和 InputManager

---

## 7. 结论

### 7.1 已解决的问题

1. ✅ 视窗鼠标拖拽旋转
2. ✅ 视窗滚轮缩放
3. ✅ 输入事件正确传递
4. ✅ 相机控制器正确附加

### 7.2 遗留问题

1. 🔴 `Window` 和 `InputManager` 架构耦合
2. 🟡 滚轮输入的时序仍需要 `update()` 在 `on_update()` 之前
3. 🟢 缺乏输入系统测试覆盖

### 7.3 下一步行动

1. 评估事件总线架构的可行性
2. 为输入系统添加单元测试
3. 考虑引入 ECS 架构简化相机控制

---

**审查人**: iFlow CLI  
**审查日期**: 2026-03-15

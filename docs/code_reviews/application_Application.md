# 代码审查报告：Application

**文件路径**

- `include/application/app/Application.hpp`
- `src/application/app/Application.cpp`

---

## 功能概述

`ApplicationBase` 是整个引擎的应用层基类，负责管理以下核心系统的生命周期：

- **Window**：GLFW 窗口创建与事件回调注册
- **DeviceManager**：Vulkan 设备初始化
- **SwapChain**：交换链创建与 resize 重建
- **InputManager**：输入系统初始化与每帧更新

主循环顺序为：`poll_events → update_input → on_update → on_render`，通过虚函数 `on_update` / `on_render` /
`on_window_resize` 供派生类覆写。

---

## 关键设计

| 特性           | 说明                                                    |
|--------------|-------------------------------------------------------|
| RAII         | Window/Device/SwapChain 均通过 `unique_ptr` 管理           |
| 虚函数钩子        | `on_update`, `on_render`, `on_window_resize` 允许派生类扩展  |
| 帧时间统计        | 通过 `std::chrono::high_resolution_clock` 计算 delta time |
| 窗口 resize 回调 | 注册 GLFW `glfwSetFramebufferSizeCallback`              |

---

## 潜在问题

### 🔴 高风险

1. **双重 SwapChain recreate**  
   `ApplicationBase::on_window_resize()` 默认实现调用 `swap_chain_->recreate()`，而派生类
   `EditorApplication::on_window_resize()` 也可能独立调用 recreate，存在双重重建风险。若派生类未调用 `super`
   ，基类逻辑则被跳过；若调用，则可能重复 recreate。  
   **建议**：在基类中使用 Template Method 模式，明确哪部分由基类负责，哪部分由派生类负责。

### 🟡 中风险

2. **InputManager 初始化顺序**  
   `InputManager` 依赖 `Window` 已初始化并拿到 GLFW handle，但初始化顺序由代码位置隐式决定，缺少显式依赖声明。

3. **delta time 精度问题**  
   `delta_time` 转换为 `float` 时，对于极长运行时（>24 小时）可能溢出或精度下降。

### 🟢 低风险

4. **`running_` 标志的线程安全**  
   若将来在多线程场景中设置 `running_ = false`，缺少 `std::atomic` 保护。

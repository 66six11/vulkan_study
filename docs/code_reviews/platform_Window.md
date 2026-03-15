# 代码审查报告：Window

**文件路径**

- `include/platform/windowing/Window.hpp`
- `src/platform/windowing/Window.cpp`

---

## 功能概述

`Window` 类封装 GLFW 窗口，采用 **Pimpl 惯用法** 隐藏实现细节：

- **窗口创建**：通过 `WindowConfig` 配置宽高、标题、全屏模式
- **事件回调**：注册 resize、键盘、鼠标按钮、滚轮回调
- **Vulkan Surface 创建**：`create_surface()` 通过 `glfwCreateWindowSurface` 创建
- **V-Sync**：`set_vsync()` 接口存在但为空实现（由 SwapChain 负责）

---

## 关键设计

| 特性       | 说明                                          |
|----------|---------------------------------------------|
| Pimpl 模式 | 将 GLFW 依赖封装在 `.cpp`，减少头文件污染                 |
| 回调注册     | 通过 `std::function` 存储，解耦调用方                 |
| RAII     | 析构时调用 `glfwDestroyWindow` 和 `glfwTerminate` |

---

## 潜在问题

### 🔴 高风险

1. **多窗口场景崩溃**  
   每次 `Window` 构造都调用 `glfwInit()`，析构时调用 `glfwTerminate()`。如果创建多个 `Window` 实例，第一个销毁时会终止所有窗口的
   GLFW 状态，导致其他窗口崩溃。  
   **建议**：使用引用计数或全局单例管理 GLFW 初始化/终止。

### 🟡 中风险

2. **`set_vsync()` 为空实现但未标注**  
   接口存在但实际无效，调用方可能误以为已生效。  
   **建议**：在函数体中添加注释说明"由 SwapChain 的 present mode 控制"，并可选地抛出警告日志。

3. **GLFW 回调中的悬空指针风险**  
   GLFW 回调通过 `glfwSetWindowUserPointer` 存储 `Window*`，若 `Window` 对象已销毁但 GLFW 仍触发回调，会访问悬空指针。

### 🟢 低风险

4. **全屏模式实现不完整**  
   `WindowConfig::fullscreen` 字段存在，但全屏切换逻辑（`glfwSetWindowMonitor`）在实现中缺失。

# 代码审查报告：Editor

**文件路径**

- `include/editor/Editor.hpp`
- `src/editor/Editor.cpp`

---

## 功能概述

`Editor` 类封装了编辑器主框架逻辑：

- **ImGui 生命周期管理**：初始化 `ImGuiManager`，在每帧调用 `new_frame()` / `render()`
- **Viewport resize 回调**：注册 `Viewport::resize_callback`，响应 ImGui 面板尺寸变化
- **命令池管理**：内部分配 `VkCommandPool` 用于一次性命令提交
- **场景渲染接口（已废弃）**：`render_scene()` 现返回 `VK_NULL_HANDLE`，实际渲染移至 `main.cpp`

---

## 关键设计

| 特性          | 说明                                                  |
|-------------|-----------------------------------------------------|
| ImGui 集成    | 通过 `ImGuiManager` 隔离 ImGui Vulkan 后端细节              |
| Viewport 回调 | 用 `std::function` 注册 resize 回调，解耦 Editor 与 Viewport |
| 命令池         | RAII 分配，析构时自动销毁                                     |

---

## 潜在问题

### 🔴 高风险

1. **`render_scene()` 已废弃但仍保留接口**  
   `render_scene()` 返回 `VK_NULL_HANDLE`，调用方若不检查返回值直接使用会触发 Vulkan 错误。  
   **建议**：标记 `[[deprecated]]` 并在文档中说明迁移路径，或彻底删除。

### 🟡 中风险

2. **命令池冗余**  
   `Editor` 内部分配了命令池，但实际渲染命令由 `main.cpp` 中的命令缓冲管理器负责，`Editor` 的命令池在当前版本中几乎未被使用，形成资源浪费。

3. **Viewport resize 回调的线程安全**  
   `resize_callback_` 为 `std::function`，若在渲染线程中触发 resize，而主线程同时重置回调，会产生竞争条件。

### 🟢 低风险

4. **`initialize()` 未检查 DeviceManager 有效性**  
   若传入空的 `shared_ptr<DeviceManager>`，会在后续 Vulkan 调用时崩溃，缺乏前置检查。

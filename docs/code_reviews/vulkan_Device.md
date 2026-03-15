# 代码审查报告：Device

**文件路径**

- `include/vulkan/device/Device.hpp`
- `src/vulkan/device/Device.cpp`

---

## 功能概述

`DeviceManager` 管理 Vulkan 核心设备对象生命周期：

- **Instance 创建**：包含验证层、调试工具扩展
- **Physical Device 选择**：`select_physical_device()` 选择合适的 GPU
- **Logical Device 创建**：分配 Graphics / Present 队列
- **内存工具**：`find_memory_type()` 查找合适的内存类型
- **调试回调**：注册 `VkDebugUtilsMessengerEXT`

---

## 关键设计

| 特性      | 说明                                      |
|---------|-----------------------------------------|
| RAII    | Instance/Device/DebugMessenger 在析构时按序销毁 |
| 验证层条件编译 | `NDEBUG` 宏控制是否启用验证层                     |
| 队列族发现   | 支持 Graphics/Present 队列族分离               |

---

## 潜在问题

### 🔴 高风险

1. **`supports_feature()` 硬编码返回 `true`**  
   所有特性查询均返回 `true`（占位符），导致调用方无法得知设备是否真正支持某功能，可能在不支持的设备上触发 Vulkan 验证错误。

2. **`check_device_support()` 未被 `select_physical_device()` 调用**  
   物理设备选择时没有验证必要的特性和扩展支持（如 Swapchain 扩展），可能在不支持的设备上初始化成功但后续崩溃。

### 🟡 中风险

3. **实例扩展仅包含 Win32 平台扩展**  
   `create_instance()` 中扩展列表硬编码了 `VK_KHR_WIN32_SURFACE_EXTENSION_NAME`，在 Linux/macOS 下编译会失败。  
   **建议**：使用 `glfwGetRequiredInstanceExtensions()` 动态获取平台扩展。

4. **`graphics_queue_family_` 硬编码为 0**  
   部分代码中假定图形队列族为 0，未从实际查询结果中读取，在部分专业 GPU 上可能不成立。

### 🟢 低风险

5. **验证层错误消息过于简单**  
   调试回调只输出 message，缺少完整的调用栈或对象名称信息，建议添加 `VK_EXT_debug_utils` 对象命名支持。

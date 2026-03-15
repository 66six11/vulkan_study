# 代码审查报告：VulkanError

**文件路径**

- `include/vulkan/utils/VulkanError.hpp`

---

## 功能概述

Vulkan 错误处理工具：

- **`VulkanError`**：继承 `std::runtime_error`，携带 `VkResult` 错误码，格式化消息包含文件名和行号
- **`result_to_string()`**：将 `VkResult` 枚举转换为可读字符串
- **`VK_CHECK` 宏**：一行代码检查 Vulkan 调用结果，失败时抛出带位置信息的异常
- **`VK_CHECK_MSG` 宏**：带自定义消息的版本

---

## 关键设计

| 特性   | 说明                               |
|------|----------------------------------|
| 异常集成 | 继承标准异常，与 C++ 异常机制无缝集成            |
| 位置信息 | `__FILE__` / `__LINE__` 提供精确错误定位 |
| 宏简化  | `VK_CHECK` 避免重复的错误检查样板代码         |

---

## 潜在问题

### 🟡 中风险

1. **`result_to_string()` 缺少新 VkResult 值**  
   函数仅覆盖了基础的错误码，缺少 Vulkan 1.2/1.3 新增的错误码（如 `VK_ERROR_COMPRESSION_EXHAUSTED_EXT`、
   `VK_PIPELINE_COMPILE_REQUIRED` 等），这些情况会返回 `"VK_UNKNOWN_ERROR"`，降低调试效率。

2. **`VK_CHECK` 宏不支持 `VK_SUBOPTIMAL_KHR` 处理**  
   `VK_SUBOPTIMAL_KHR` 是非错误的非成功结果（值为正），直接用 `!= VK_SUCCESS` 检查会将其视为错误抛出，而正确处理应该只是触发
   SwapChain recreate。  
   **建议**：提供 `VK_CHECK_SWAPCHAIN` 宏或类似变体，对 suboptimal 情况特殊处理。

### 🟢 低风险

3. **`format_message()` 使用字符串拼接性能较低**  
   `format_message` 使用 `+` 拼接字符串，在高频错误路径下（虽然错误本身不应高频）性能略差。可改用 `std::ostringstream` 或
   `std::format`（C++20）。

4. **头文件中包含完整实现**  
   `VulkanError` 的所有方法均在头文件中定义，包含此头文件的翻译单元都会内联这些函数，轻微增加编译时间。

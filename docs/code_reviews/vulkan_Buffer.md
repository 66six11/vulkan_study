# 代码审查报告：Buffer

**文件路径**

- `include/vulkan/resources/Buffer.hpp`
- `src/vulkan/resources/Buffer.cpp`

---

## 功能概述

`Buffer` 类封装 Vulkan 缓冲区和设备内存：

- **RAII 管理**：构造时分配，析构时释放 `VkBuffer` + `VkDeviceMemory`
- **数据操作**：`map/unmap`, `write`, `read`, `copy_from`, `flush`, `invalidate`
- **`BufferBuilder`**：流式 API 配置内存属性
- **`BufferManager`**：工厂方法创建不同用途缓冲区（vertex/index/uniform/staging）

---

## 关键设计

| 特性         | 说明                                                           |
|------------|--------------------------------------------------------------|
| RAII 异常安全  | 构造函数内部使用局部 guard 结构确保异常时自动清理 `VkBuffer`                      |
| 内存属性分离     | 使用者通过 `VkMemoryPropertyFlags` 控制 host-visible / device-local |
| Builder 模式 | `BufferBuilder` 链式调用，避免构造函数参数过多                              |

---

## 潜在问题

### 🔴 高风险

1. **`copy_from()` 使用 CPU memcpy，不支持 Device-local 内存**  
   `copy_from()` 直接 `map()` 两块内存后 `memcpy`，但 device-local 缓冲（`VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT`）无法被 CPU
   映射。对 device-local 缓冲调用此函数会导致 `vkMapMemory` 失败。  
   **建议**：添加是否 host-visible 的判断，非 host-visible 时改用命令缓冲的 `vkCmdCopyBuffer`。

2. **`map()` 不检查已映射状态**  
   若 `map()` 被调用两次而未 `unmap()`，会触发 Vulkan validation error（同一内存范围不能重复映射）。  
   **建议**：添加 `is_mapped_` 检查，已映射时直接返回已有指针。

### 🟡 中风险

3. **`BufferManager::get_stats()` 始终返回空统计**  
   统计功能未实现，无法通过此接口监控内存使用情况。

4. **`destroy_buffer()` 仅重置 shared_ptr，依赖引用计数**  
   `destroy_buffer(shared_ptr<Buffer>)` 通过 `buffer.reset()` 释放，实际上只是减少引用计数，若其他地方还持有该
   shared_ptr，缓冲不会真正释放。接口语义容易被误解。

### 🟢 低风险

5. **未使用 VMA（Vulkan Memory Allocator）**  
   手动管理 `VkDeviceMemory` 在大量小缓冲场景下会超出设备分配次数限制（通常 4096 次），建议集成 VMA 进行内存池化管理。

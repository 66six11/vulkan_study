# 代码审查报告：Synchronization

**文件路径**

- `include/vulkan/sync/Synchronization.hpp`
- `src/vulkan/sync/Synchronization.cpp`

---

## 功能概述

Vulkan 同步原语封装：

- **`Fence`**：CPU-GPU 同步，支持 `wait()` / `reset()` / `is_signaled()`
- **`Semaphore`**：GPU-GPU 同步（binary semaphore）
- **`TimelineSemaphore`**：时间线信号量（占位符，未实现）
- **`Event`**：Pipeline 内事件同步
- **`SynchronizationManager`**：批量创建 / 销毁同步原语
- **`FrameSyncManager`**：多帧同步管理（per-frame fence + semaphore）

---

## 关键设计

| 特性   | 说明                                                                 |
|------|--------------------------------------------------------------------|
| RAII | 所有同步原语在析构时自动销毁                                                     |
| 按帧管理 | `FrameSyncManager` 支持 `MAX_FRAMES_IN_FLIGHT` 帧并发                   |
| 分离策略 | in-flight fence 按帧，image-available semaphore 按帧，render-finished 按帧 |

---

## 潜在问题

### 🔴 高风险

1. **`TimelineSemaphore` 完全未实现**  
   注释说"需要 Vulkan 1.2+"，但项目目标是 Vulkan 1.3+（Timeline Semaphore 已是核心特性）。实际创建的是普通 binary
   semaphore，调用 `signal()` / `wait()` 等方法时会产生无效操作或崩溃。  
   **建议**：添加 `VkSemaphoreTypeCreateInfo` 并实现真正的 timeline semaphore 操作。

### 🟡 中风险

2. **`Fence::wait()` 超时处理**  
   默认超时为 `UINT64_MAX`（无限等待），在 GPU hang 时会导致程序永久阻塞。  
   **建议**：设置合理超时（如 3 秒），超时后记录错误并尝试恢复。

3. **`FrameSyncManager` 未处理 `VK_SUBOPTIMAL_KHR`**  
   `acquire_next_image()` 返回 `VK_SUBOPTIMAL_KHR` 时，当前代码视为成功，但此时应触发 SwapChain recreate。

### 🟢 低风险

4. **`SynchronizationManager::cleanup()` 前未等待 idle**  
   批量销毁同步原语前未调用 `vkDeviceWaitIdle`，在 GPU 仍在使用这些原语时销毁可能触发验证错误。

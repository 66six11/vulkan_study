# 代码审查报告：UniformBuffer

**文件路径**

- `include/vulkan/resources/UniformBuffer.hpp`

---

## 功能概述

`UniformBuffer<T>` 是模板类，为每帧创建一个 host-visible uniform buffer：

- **多帧支持**：`frame_count` 个独立缓冲，支持 CPU-GPU 并发
- **持久映射**：构造时 `vkMapMemory`，析构时 `vkUnmapMemory`，避免每帧 map/unmap 开销
- **帧索引管理**：`set_frame(frame)` 切换当前帧，`update(data)` 写入当前帧缓冲
- **直接访问**：`current_buffer()` / `buffer(frame)` 提供 VkBuffer 句柄

---

## 关键设计

| 特性     | 说明                                      |
|--------|-----------------------------------------|
| 持久映射   | HOST_COHERENT 内存无需手动 flush，直接 memcpy 即可 |
| 模板化    | 类型安全，无需手动计算 sizeof                      |
| 多帧独立缓冲 | 避免 CPU/GPU 同时访问同一缓冲造成的竞争                |

---

## 潜在问题

### 🔴 高风险

1. **析构时 Unmap/Free 顺序问题**  
   若 `device_` 在 `UniformBuffer` 析构之前被销毁（如局部变量顺序问题），析构时调用 `vkUnmapMemory(device_->device(), ...)`
   会解引用已销毁的 `DeviceManager`，导致崩溃。  
   **建议**：确保 `UniformBuffer` 生命周期不超过 `DeviceManager`，或使用 `weak_ptr` 检查有效性。

### 🟡 中风险

2. **`frame_count_` 未进行上限检查**  
   若传入 `frame_count = 0`，循环不执行，但 `buffers_` / `memories_` / `mapped_data_` 为空 vector，后续 `update()` 访问
   `[current_frame_]` 时下标越界（UB）。  
   **建议**：在构造函数中添加 `assert(frame_count > 0)`。

3. **`get_data()` 通过 memcpy 读取，语义不直观**  
   通过 `memcpy` 从映射内存读取数据，在 non-coherent 内存上需要先调用 `vkInvalidateMappedMemoryRanges`，但 `UniformBuffer`
   假设使用 `HOST_COHERENT` 内存（未明确文档化约束）。

### 🟢 低风险

4. **仅实现在头文件（模板类），每个翻译单元都会实例化完整实现**  
   对于大型项目，建议使用显式模板实例化（`extern template`）减少编译时间。

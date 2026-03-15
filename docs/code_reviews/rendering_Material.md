# 代码审查报告：Material

**文件路径**

- `include/rendering/material/Material.hpp`
- `src/rendering/material/Material.cpp`

---

## 功能概述

`Material` 管理 Vulkan 渲染材质的完整生命周期：

- **多 RenderPass 管线缓存**：`unordered_map<VkRenderPass, pipeline>` 按 RenderPass 缓存管线
- **Uniform Buffer**：持有 `MaterialUBO`（基础颜色、roughness、metallic 等 PBR 参数）
- **Descriptor Set**：绑定 UBO 和纹理到 descriptor set
- **纹理系统**：支持多纹理绑定，提供默认白色 1x1 纹理 fallback
- **参数 API**：`set_float`, `set_vec3`, `set_vec4` 等高层参数设置

---

## 关键设计

| 特性              | 说明                             |
|-----------------|--------------------------------|
| 多 RenderPass 支持 | 同一材质可在不同 RenderPass 上下文中使用不同管线 |
| 懒加载管线           | `bind(render_pass)` 时懒构建对应管线   |
| 默认纹理            | 所有未绑定的纹理槽使用白色 1x1 纹理占位         |

---

## 潜在问题

### 🔴 高风险

1. **`set_vec2`, `set_int`, `set_bool` 声明但未实现**  
   这三个方法在头文件中声明，但 `.cpp` 中缺少实现，调用时会产生链接错误（linker error）。

2. **`bind()` 热路径中触发管线编译**  
   `bind(render_pass)` 每次被调用时若该 RenderPass
   的管线尚未构建，会在渲染热路径中同步编译管线，导致该帧出现严重卡顿（几毫秒到几百毫秒不等）。  
   **建议**：在材质加载时预编译所有可能用到的管线，或使用后台编译。

3. **`create_default_white_texture()` 未使用 RAII Buffer 类**  
   手动管理 `staging_buffer` / `staging_memory`，若中途抛出异常，staging 资源会泄漏。  
   **建议**：使用 `Buffer` RAII 类管理 staging buffer。

### 🟡 中风险

4. **描述符集未支持 dynamic offset**  
   UBO 使用固定 binding，不支持多帧动态偏移，在多帧并发时可能产生数据竞争。

5. **Sampler 在 Material 内硬编码创建**  
   每个 Material 都创建独立的 `VkSampler`，相同参数的 Sampler 无法共享，在大量材质时浪费资源。

### 🟢 低风险

6. **`MaterialUBO` 结构体对齐未验证**  
   若 `MaterialUBO` 成员不满足 std140 对齐规则，shader 读取的数值会不正确，但编译器不会报错。

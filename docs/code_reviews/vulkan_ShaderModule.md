# 代码审查报告：ShaderModule

**文件路径**

- `include/vulkan/pipelines/ShaderModule.hpp`
- `src/vulkan/pipelines/ShaderModule.cpp`

---

## 功能概述

`ShaderModule` 封装 `VkShaderModule` RAII 对象，支持三种创建方式：

- **从 SPIR-V uint32_t 数组创建**
- **从原始字节（uint8_t*）创建**（自动对齐到 4 字节）
- **从文件路径加载**：`load_spirv_from_file()` 读取 `.spv` 文件并验证 Magic Number

附带 `ShaderStageInfo` 辅助结构体，方便创建 `VkPipelineShaderStageCreateInfo`。

---

## 关键设计

| 特性        | 说明                            |
|-----------|-------------------------------|
| RAII      | 析构时自动 `vkDestroyShaderModule` |
| SPIR-V 验证 | 检查 Magic Number `0x07230203`  |
| 字节对齐处理    | 原始字节输入时自动 padding 到 4 字节对齐    |
| 完整移动语义    | 移动后原对象 `module_` 清零           |

---

## 潜在问题

### 🟡 中风险

1. **`load_spirv_from_file()` 不支持相对路径解析**  
   文件路径必须是可直接打开的路径，不支持从配置的 shader 目录相对查找，在不同工作目录下运行时容易失败。  
   **建议**：与 `ShaderManager` 集成，统一路径解析。

2. **从字节创建时 `code_size` 计算逻辑有歧义**  
   `size_t code_size = (size + 3) / 4` 是 uint32_t 个数（向上取整），但 `codeSize` 字段传入的是原始字节数 `size`，确保
   SPIR-V 规范要求的 4 字节对齐的实际字节数为 `code.size() * 4`，而非 `size`。  
   **建议**：添加注释说明此处的设计意图，并验证与 Vulkan 规范的一致性。

### 🟢 低风险

3. **`ShaderStageInfo::specialization` 生命周期**  
   `ShaderStageInfo::specialization` 是裸指针，指向外部管理的 `VkSpecializationInfo`，若外部对象先销毁而 `ShaderStageInfo`
   仍被使用，会产生悬空指针。

4. **无缓存机制**  
   每次创建 `ShaderModule` 都重新读取文件和创建 Vulkan 对象，频繁重建相同着色器时效率低。建议与 `ShaderManager` 配合实现缓存。

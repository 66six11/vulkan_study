# 代码审查报告：Pipeline

**文件路径**

- `include/vulkan/pipelines/Pipeline.hpp`
- `src/vulkan/pipelines/Pipeline.cpp`

---

## 功能概述

Vulkan 图形管线封装：

- **`PipelineLayout`**：管理 `VkPipelineLayout` RAII 封装，包含描述符集布局和 push constant 范围
- **`GraphicsPipeline`**：完整图形管线创建，支持动态 Viewport/Scissor、深度测试、混合模式
- **`PipelineCache`**：`VkPipelineCache` 包装，支持磁盘持久化

---

## 关键设计

| 特性         | 说明                                         |
|------------|--------------------------------------------|
| Builder 模式 | `GraphicsPipelineBuilder` 链式配置管线状态         |
| 动态状态       | Viewport / Scissor 设为动态，支持窗口 resize 无需重建管线 |
| 几何着色器支持    | 可选配置几何着色器阶段                                |
| RAII       | 析构时自动销毁 `VkPipeline`                       |

---

## 潜在问题

### 🟡 中风险

1. **`GraphicsPipeline` 移动后 `layout_` 不清零**  
   移动构造/赋值后，原对象的 `layout_` 成员未置为 `VK_NULL_HANDLE`，若原对象析构时尝试销毁该
   layout，会产生双重释放（虽然析构有空值检查，但原对象持有非 null 的已无效句柄）。

2. **`PipelineCache` 序列化兼容性**  
   从磁盘加载的 pipeline cache 数据在驱动版本或硬件更换后会无效，但代码未做版本校验，可能在 Vulkan 验证层下产生警告。  
   **建议**：保存 device UUID 和驱动版本，加载时验证兼容性。

3. **`GraphicsPipelineBuilder` 未验证必要字段**  
   若忘记设置 vertex shader 或 render pass，`build()` 时 Vulkan 会报错，但错误信息不够友好。  
   **建议**：在 `build()` 前添加前置条件检查并输出明确错误。

### 🟢 低风险

4. **固定的混合因子**  
   Alpha 混合参数（`srcAlphaBlendFactor` 等）硬编码，无法通过 Builder 自定义透明度混合模式。

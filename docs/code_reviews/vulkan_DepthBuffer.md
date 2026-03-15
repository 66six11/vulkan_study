# 代码审查报告：DepthBuffer

**文件路径**

- `include/vulkan/resources/DepthBuffer.hpp`
- `src/vulkan/resources/DepthBuffer.cpp`

---

## 功能概述

`DepthBuffer` 封装深度缓冲图像，专用于深度/模板附件：

- **自动格式选择**：`find_depth_format()` 从候选格式（D32_SFLOAT → D32_SFLOAT_S8 → D24_UNORM_S8）中选择硬件支持的最优格式
- **图像创建**：`create_image()` 创建 `DEPTH_STENCIL_ATTACHMENT_BIT` 用途图像
- **视图创建**：`create_view()` 创建 `VK_IMAGE_ASPECT_DEPTH_BIT` 视图
- **移动语义**：完整的移动构造/移动赋值实现，正确置空原对象句柄

---

## 关键设计

| 特性     | 说明                            |
|--------|-------------------------------|
| RAII   | 析构时按序销毁 View → Image → Memory |
| 格式选择   | 运行时查询硬件支持，选择最优深度格式            |
| 完整移动语义 | 移动后原对象句柄清零，避免双重释放             |

---

## 潜在问题

### 🟡 中风险

1. **`find_depth_format()` 候选格式选择策略**  
   D32_SFLOAT 优先于 D32_SFLOAT_S8，但后者支持模板测试。若后续添加模板效果（如 stencil shadow），需要重新评估格式选择策略。

2. **深度图像不支持采样（无 `SAMPLED_BIT`）**  
   `VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT` 中缺少 `VK_IMAGE_USAGE_SAMPLED_BIT`，导致深度图像无法被着色器采样（如
   shadow map 等效果需要此特性）。  
   **建议**：添加 `SAMPLED_BIT` 以支持未来的深度采样需求。

3. **`vkBindImageMemory` 返回值未检查**  
   `create_image()` 中 `vkBindImageMemory` 调用未检查返回值，绑定失败时会静默忽略。

### 🟢 低风险

4. **`find_depth_format` 作为静态方法调用时需要 shared_ptr**  
   该静态方法签名为 `find_depth_format(shared_ptr<DeviceManager>)`，调用时传值拷贝 shared_ptr，存在不必要的引用计数开销；改为
   `const DeviceManager&` 或 `DeviceManager*` 更高效。

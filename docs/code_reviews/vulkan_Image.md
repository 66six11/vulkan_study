# 代码审查报告：Image

**文件路径**

- `include/vulkan/resources/Image.hpp`
- `src/vulkan/resources/Image.cpp`

---

## 功能概述

`Image` 类封装 `VkImage` + `VkImageView` + `VkDeviceMemory`：

- **创建**：支持任意格式、mip levels、array layers
- **视图管理**：`create_view()` 可重新创建 ImageView
- **布局追踪**：`current_layout_` 追踪当前图像布局
- **占位方法**：`transition_layout()`, `generate_mipmaps()`, `upload_data()`, `download_data()` 均为占位符

---

## 关键设计

| 特性           | 说明                               |
|--------------|----------------------------------|
| RAII         | 析构时按序销毁 View → Image → Memory    |
| 布局状态追踪       | `current_layout_` 字段记录当前布局（软件状态） |
| Builder 模式   | `ImageBuilder` 提供链式配置            |
| ImageManager | 工厂方法创建 color/depth/texture 图像    |

---

## 潜在问题

### 🔴 高风险

1. **`transition_layout()` 不实际执行布局转换**  
   `transition_layout(new_layout)` 仅更新软件状态 `current_layout_`，不提交任何 Vulkan barrier
   命令。调用者若期望此函数真正执行布局转换，会导致图像布局与软件状态不同步，引发 validation errors 或渲染错误。  
   **建议**：此函数应要求命令缓冲参数，或重命名为 `set_tracked_layout()` 表明其只是状态追踪。

2. **`upload_data()` / `download_data()` 为完全空实现**  
   这两个函数是空占位符，调用后不会发生任何数据传输，但不返回错误，调用者无法感知失败。

### 🟡 中风险

3. **`generate_mipmaps()` 为空占位符**  
   `ImageBuilder` 支持配置 `mip_levels > 1`，但 `generate_mipmaps()` 未实现，创建多 mip 级别的图像后无法自动生成 mipmap
   数据，低级别 mip 为未初始化内存。

4. **`create_view()` 使用时强制使用 `VK_IMAGE_ASPECT_COLOR_BIT`**  
   构造函数中固定传入 `VK_IMAGE_ASPECT_COLOR_BIT` 创建默认视图，对深度图像会出错（应为 `VK_IMAGE_ASPECT_DEPTH_BIT`）。

### 🟢 低风险

5. **`ImageBuilder::depth()` 参数未使用**  
   `ImageBuilder` 有 `depth()` 方法设置 `depth_` 字段，但 `build()` 中传给 `Image` 构造函数时未传递该值（`Image` 固定为
   2D），导致 3D 图像无法通过 Builder 创建。

# 代码审查报告：TextureLoader

**文件路径**

- `include/rendering/resources/TextureLoader.hpp`
- `src/rendering/resources/TextureLoader.cpp`

---

## 功能概述

`TextureLoader` 使用 `stb_image` 从文件加载纹理，并上传到 GPU：

- **文件加载**：`stbi_load` 解码 PNG/JPG/BMP 等格式
- **Mipmap 生成**：通过 `vkCmdBlitImage` 生成 mipmap chain
- **路径解析**：`resolve_path()` 尝试多个搜索路径
- **Staging 流程**：手动管理 staging buffer，提交一次性命令缓冲

---

## 关键设计

| 特性        | 说明                           |
|-----------|------------------------------|
| STB Image | 零依赖图像解码                      |
| 完整 Mipmap | blit 方式生成，GPU 端并行高效          |
| 格式固定      | 强制加载为 `STBI_rgb_alpha`（4 通道） |

---

## 潜在问题

### 🔴 高风险

1. **`resolve_path()` 硬编码绝对路径 `D:/TechArt/Vulkan/`**  
   与 `MaterialLoader` 同样的问题，在非开发机器上此路径无效，严重限制可移植性。

2. **Staging buffer 使用裸 Vulkan API，无 RAII 保护**  
   `create_image_from_data()` 手动创建/销毁 staging buffer 和内存，若 `vkAllocateMemory` 之后的任意操作抛出异常（如
   `Image` 构造失败），staging 资源会泄漏。  
   **建议**：使用项目的 `Buffer` RAII 类管理 staging 资源。

3. **`vkQueueWaitIdle` 在纹理加载路径**  
   每次纹理加载都调用 `vkQueueWaitIdle`，阻塞 CPU 等待 GPU
   完成。加载多纹理时（如材质系统），每张纹理都会阻塞一次，总体加载时间显著增加。  
   **建议**：批量提交或使用 fence 代替 `WaitIdle`。

### 🟡 中风险

4. **queue_family_index 硬编码为 0**  
   `VkCommandPoolCreateInfo.queueFamilyIndex = 0`，假设 graphics 队列族为 0，与 `SwapChain` 中同样的问题。

5. **格式参数 `format` 被 `(void)format` 忽略**  
   `load_texture(path, format, ...)` 的 `format` 参数被注释为 "Currently always uses VK_FORMAT_R8G8B8A8_UNORM"
   并忽略，接口设计具有误导性。

### 🟢 低风险

6. **`calculate_mip_levels()` 计算结果为 `log2(max_dim) + 1`**  
   函数通过位移循环计算，结果等价于 `floor(log2(max(width, height))) + 1`，逻辑正确，但循环计算不如直接使用
   `std::bit_width`（C++20）直观。

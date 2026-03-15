# 代码审查报告：ImGuiManager

**文件路径**

- `include/editor/ImGuiManager.hpp`
- `src/editor/ImGuiManager.cpp`

---

## 功能概述

`ImGuiManager` 负责 ImGui 在 Vulkan 后端的完整生命周期：

- **初始化**：创建 ImGui 上下文、设置 Vulkan 后端 (`ImGui_ImplVulkan_Init`)
- **描述符池**：为 ImGui 分配专用 `VkDescriptorPool`（支持多种描述符类型）
- **每帧驱动**：`new_frame()` / `render()` / `end_frame()`
- **UI 面板**：`draw_material_panel()` 提供材质参数调节界面（roughness/metallic/albedo）
- **字体加载**：支持从文件加载中文字体

---

## 关键设计

| 特性          | 说明                                 |
|-------------|------------------------------------|
| Vulkan 后端隔离 | 所有 `imgui_impl_vulkan.h` 细节封装在此类内部 |
| 描述符池独立      | ImGui 使用专属描述符池，不与渲染管线共享            |
| 字体系统        | 支持 DPI 缩放和中文字体 fallback            |

---

## 潜在问题

### 🔴 高风险

1. **材质参数与实际 `Material` 对象解耦**  
   `draw_material_panel()` 中，`roughness`、`metallic`、`albedo` 均为**静态局部变量**，与真实的 `Material` 对象完全无关联。UI
   显示的数值变化不会影响渲染结果，反之渲染中的材质值也不会反映到 UI。  
   **建议**：通过回调或 `weak_ptr<Material>` 将 UI 状态与材质对象双向绑定。

### 🟡 中风险

2. **误导性注释 `// Not used`**  
   `(void)render_pass` 注释说 "Not used"，但后续代码仍将 `render_pass` 赋值给 `init_info.RenderPass`，注释严重误导阅读者，应删除。

3. **描述符池容量设计过宽**  
   `maxSets = 1000 * 11 = 11000`，每种类型描述符各 1000 个，过于笼统，在低显存设备上可能造成显存压力。

### 🟢 低风险

4. **字体文件路径硬编码**  
   中文字体路径为硬编码字符串，在非 Windows 平台或不同安装路径下会加载失败，应改为可配置路径。

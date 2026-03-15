# Vulkan Engine 架构重构方案

**版本**: 1.0  
**日期**: 2026-03-15  
**状态**: Phase 1 & 2 完成

---

## **概述**

基于代码审查报告，执行三阶段重构，解决架构问题。

---

## **Phase 1: 核心架构修复 (已完成)**

### **1.1 RenderPassManager**

**新增文件**:

- `include/vulkan/pipelines/RenderPassManager.hpp`
- `src/vulkan/pipelines/RenderPassManager.cpp`

**职责**:

- 集中创建和管理 RenderPass
- 提供缓存机制避免重复创建
- 支持多种 RenderPass 类型（Present、Off-screen、Shadow）

**使用示例**:

```cpp
auto render_pass_mgr = std::make_shared<RenderPassManager>(device);

// 获取 Present RenderPass（带深度）
VkRenderPass rp = render_pass_mgr->get_present_render_pass_with_depth(
    swap_chain_->format(),
    depth_buffer_->format()
);
```

### **1.2 CameraController**

**新增文件**:

- `include/rendering/camera/CameraController.hpp`
- `src/rendering/camera/CameraController.cpp`

**职责**:

- 解耦输入处理与相机逻辑
- 支持拖拽旋转和滚轮缩放
- 可配置灵敏度和按键

**使用示例**:

```cpp
auto controller = std::make_unique<OrbitCameraController>();
controller->attach_camera(camera_);
controller->attach_input_manager(input_manager_);
controller->attach_viewport(viewport_);

// 在 update 中调用
controller->update(delta_time);
```

### **1.3 修复 Camera Y轴翻转**

**修改文件**:

- `include/core/math/Camera.hpp`

**变更**:

```cpp
// 修改前
proj[1][1] *= -1; // Flip Y for Vulkan

// 修改后
// 移除了 Y轴翻转，Camera 现在是 API 无关的
// 使用 CoordinateTransform 工具进行转换
```

**新增文件**:

- `include/vulkan/utils/CoordinateTransform.hpp`

**使用示例**:

```cpp
#include "vulkan/utils/CoordinateTransform.hpp"

// 在 Vulkan 后端进行坐标系转换
glm::mat4 proj = camera_.get_projection_matrix(fov, aspect, near, far);
glm::mat4 vulkan_proj = CoordinateTransform::opengl_to_vulkan_projection(proj);
```

---

## **Phase 2: Render Graph 完善 (已完成)**

### **2.1 RenderTarget（分离职责）**

**新增文件**:

- `include/rendering/resources/RenderTarget.hpp`
- `src/rendering/resources/RenderTarget.cpp`

**职责**:

- 管理颜色/深度附件（Image + ImageView）
- 提供尺寸信息
- 不参与 RenderPass/Framebuffer 管理

**对比原 SceneViewport**:
| 职责 | SceneViewport (旧) | RenderTarget (新) |
|------|-------------------|-------------------|
| Image 管理 | ✅ | ✅ |
| RenderPass 管理 | ✅ | ❌ |
| Framebuffer 管理 | ✅ | ❌ |
| ImGui 集成 | ✅ | ❌ |
| 尺寸管理 | ✅ | 部分 |

### **2.2 Viewport（简化版）**

**新增文件**:

- `include/rendering/Viewport.hpp`
- `src/rendering/Viewport.cpp`

**职责**:

- 管理显示尺寸与渲染目标尺寸的关系
- 提供宽高比计算
- 提供 ImGui 纹理ID
- 延迟 resize 处理

**对比原 SceneViewport**:

```cpp
// 原 SceneViewport（职责过重）
class SceneViewport {
    // Image 管理
    // RenderPass 管理
    // Framebuffer 管理
    // ImGui 集成
    // begin/end_render_pass
};

// 新 Viewport（职责单一）
class Viewport {
    std::shared_ptr<RenderTarget> render_target_;  // 渲染目标
    // 只负责视窗逻辑
    float aspect_ratio() const;
    void request_resize(width, height);
    ImTextureID imgui_texture_id() const;
};
```

### **2.3 ViewportRenderPass**

**新增文件**:

- `include/rendering/render_graph/ViewportRenderPass.hpp`
- `src/rendering/render_graph/ViewportRenderPass.cpp`

**职责**:

- 管理渲染到纹理的完整流程
- 控制 RenderPass 生命周期（begin/end）
- 支持子渲染通道

**使用示例**:

```cpp
// 创建 RenderTarget
auto render_target = std::make_shared<RenderTarget>();
render_target->initialize(device, {1280, 720, ...});

// 创建 Viewport
auto viewport = std::make_shared<Viewport>();
viewport->initialize(device, render_target);

// 创建 ViewportRenderPass
ViewportRenderPass::Config config{
    .name = "SceneViewport",
    .render_target = render_target,
    .clear_color = true,
    .clear_depth = true
};
auto vp_pass = std::make_unique<ViewportRenderPass>(config);

// 添加子通道
vp_pass->add_sub_pass(std::make_unique<CubeRenderPass>(...));

// 添加到 RenderGraph
render_graph_.builder().add_node(std::move(vp_pass));
render_graph_.compile();
```

---

## **Phase 3: 迁移指南（待执行）**

### **3.1 迁移 main.cpp**

**原代码**:

```cpp
// 手动管理 RenderPass
viewport_->begin_render_pass(cmd.handle());
rendering::RenderContext ctx;
ctx.render_pass = viewport_->render_pass();
ctx.framebuffer = viewport_->framebuffer();
render_graph_.execute(cmd, ctx);
viewport_->end_render_pass(cmd.handle());

// 手动处理输入
if (editor_->is_viewport_hovered() && input_manager()) {
    if (input_manager()->is_mouse_button_pressed(...)) {
        camera_.on_mouse_drag(...);
    }
}
```

**新代码**:

```cpp
// 使用 CameraController
auto camera_controller_ = std::make_unique<OrbitCameraController>();
camera_controller_->attach_camera(camera_);
camera_controller_->attach_input_manager(input_manager_);
camera_controller_->attach_viewport(viewport_);

// 在 on_update 中
camera_controller_->set_enabled(editor_->is_viewport_hovered());
camera_controller_->update(delta_time);

// 使用 ViewportRenderPass（RenderGraph 完全控制）
// 不需要手动 begin/end RenderPass
render_graph_.execute(cmd, ctx);

// 投影矩阵转换
auto proj = camera_.get_projection_matrix(fov, aspect, near, far);
auto vulkan_proj = CoordinateTransform::opengl_to_vulkan_projection(proj);
```

### **3.2 迁移 SceneViewport**

**策略**: SceneViewport 可以保留为兼容层，逐步迁移到 Viewport + RenderTarget。

```cpp
// SceneViewport 可以简化为
class SceneViewport {
    std::shared_ptr<RenderTarget> render_target_;
    std::shared_ptr<Viewport> viewport_;
    
public:
    void initialize(...) {
        render_target_ = std::make_shared<RenderTarget>();
        render_target_->initialize(device, {...});
        
        viewport_ = std::make_shared<Viewport>();
        viewport_->initialize(device, render_target_);
    }
    
    // 委托给 Viewport
    float aspect_ratio() const { return viewport_->aspect_ratio(); }
    ImTextureID imgui_texture_id() const { return viewport_->imgui_texture_id(); }
    
    // 委托给 RenderTarget
    VkImageView color_image_view() const { return render_target_->color_image_view(); }
    
    // 移除 begin/end_render_pass 方法
};
```

### **3.3 迁移 SwapChain**

**原代码**:

```cpp
swap_chain_->create_render_pass_with_depth(depth_format);
VkRenderPass rp = swap_chain_->default_render_pass();
```

**新代码**:

```cpp
auto rp_mgr = std::make_shared<RenderPassManager>(device);
VkRenderPass rp = rp_mgr->get_present_render_pass_with_depth(
    swap_chain_->format(),
    depth_format
);
```

---

## **文件变更汇总**

### **新增文件 (10个)**

| 文件                                                      | 模块        | 说明               |
|---------------------------------------------------------|-----------|------------------|
| `include/vulkan/pipelines/RenderPassManager.hpp`        | Vulkan    | RenderPass 管理器   |
| `src/vulkan/pipelines/RenderPassManager.cpp`            | Vulkan    | RenderPass 管理器实现 |
| `include/vulkan/utils/CoordinateTransform.hpp`          | Vulkan    | 坐标系转换工具          |
| `include/rendering/camera/CameraController.hpp`         | Rendering | 相机控制器            |
| `src/rendering/camera/CameraController.cpp`             | Rendering | 相机控制器实现          |
| `include/rendering/resources/RenderTarget.hpp`          | Rendering | 渲染目标             |
| `src/rendering/resources/RenderTarget.cpp`              | Rendering | 渲染目标实现           |
| `include/rendering/Viewport.hpp`                        | Rendering | 视窗类              |
| `src/rendering/Viewport.cpp`                            | Rendering | 视窗实现             |
| `include/rendering/render_graph/ViewportRenderPass.hpp` | Rendering | 视窗渲染通道           |
| `src/rendering/render_graph/ViewportRenderPass.cpp`     | Rendering | 视窗渲染通道实现         |

### **修改文件 (1个)**

| 文件                             | 变更                    |
|--------------------------------|-----------------------|
| `include/core/math/Camera.hpp` | 移除 `proj[1][1] *= -1` |

---

## **CMake 配置**

由于 CMake 使用 `file(GLOB_RECURSE)` 自动发现文件，新增文件会被自动包含到构建中，无需手动修改 CMakeLists.txt。

---

## **下一步行动**

1. **测试新组件**
    - 验证 RenderPassManager 正确创建 RenderPass
    - 验证 CameraController 正确处理输入
    - 验证 CoordinateTransform 正确转换坐标

2. **逐步迁移 main.cpp**
    - 先集成 CameraController
    - 再集成 Viewport + RenderTarget
    - 最后使用 ViewportRenderPass

3. **弃用 SceneViewport**
    - 保留兼容层一段时间
    - 完全迁移后移除 SceneViewport

4. **更新 SwapChain**
    - 移除 RenderPass 创建代码
    - 使用 RenderPassManager

---

## **验证清单**

- [x] RenderPassManager 创建成功
- [x] CameraController 创建成功
- [x] CoordinateTransform 创建成功
- [x] RenderTarget 创建成功
- [x] Viewport 创建成功
- [x] ViewportRenderPass 创建成功
- [x] Camera.hpp 修复完成
- [ ] 编译通过
- [ ] 运行测试
- [ ] 完成 main.cpp 迁移
- [ ] 完成 SceneViewport 迁移

---

**完成时间**: 2026-03-15

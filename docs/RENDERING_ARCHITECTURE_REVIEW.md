# Vulkan Engine 渲染架构设计审查报告

**审查日期**: 2026-03-14  
**审查范围**: Camera、SceneViewport、Editor 职责划分与交互设计  
**严重程度**: 🔴 高 | 🟡 中 | 🟢 低

---

## 1. 执行摘要

渲染架构存在多处**职责划分不清**的问题，特别是：

1. **Camera 与 SceneViewport 职责重叠和缺失** - 投影矩阵计算和宽高比管理分散
2. **渲染目标管理混乱** - SceneViewport 承担过多渲染状态管理职责
3. **输入处理职责分散** - 相机控制在 main.cpp 和 Editor 之间分裂
4. **Render Graph 与视窗集成不完整** - Render Graph 被绕过

**架构健康度**: 6/10

---

## 2. 核心问题：Camera 与 SceneViewport 职责冲突

### 2.1 问题描述

当前设计中，**投影矩阵计算**和**宽高比管理**被不合理地分散在多个组件中：

```cpp
// main.cpp - update_mvp_matrix()
// ❌ 宽高比计算在应用层，Camera 无法自主管理
float aspect_ratio = static_cast<float>(width_) / static_cast<float>(height_);
if (editor_ && editor_->viewport()) {
    auto display_extent = viewport->display_extent();
    aspect_ratio = static_cast<float>(display_extent.width) / 
                   static_cast<float>(display_extent.height);
}
glm::mat4 proj = camera_.get_projection_matrix(45.0f, aspect_ratio, 0.1f, 100.0f);
```

```cpp
// Camera.hpp
// ❌ Camera 被动接受宽高比，无法响应视窗变化
class OrbitCamera {
    glm::mat4 get_projection_matrix(float fov, float aspect_ratio, 
                                     float near_plane, float far_plane) const {
        glm::mat4 proj = glm::perspective(glm::radians(fov), aspect_ratio, 
                                          near_plane, far_plane);
        proj[1][1] *= -1; // ❌ Vulkan Y轴翻转隐藏在Camera内部
        return proj;
    }
};
```

### 2.2 职责划分分析

| 职责          | 当前归属            | 应该归属                       | 问题           |
|-------------|-----------------|----------------------------|--------------|
| 宽高比计算       | `main.cpp`      | `SceneViewport` 或 `Camera` | 职责泄露到应用层     |
| 投影矩阵计算      | `Camera`        | `Camera`                   | ✅ 正确         |
| Vulkan Y轴翻转 | `Camera`        | `渲染后端`                     | 平台相关代码污染业务逻辑 |
| 视窗尺寸获取      | `SceneViewport` | `SceneViewport`            | ✅ 正确         |
| 渲染目标尺寸      | `SceneViewport` | `SceneViewport`            | ✅ 正确         |

### 2.3 设计缺陷详解

#### 🔴 缺陷 1: Camera 与视窗生命周期解耦

**问题**:

```cpp
// Camera 不知道视窗的存在
class OrbitCamera {
    // 没有与 SceneViewport 的关联
    // 每次渲染都需要外部传入宽高比
};

// 每次渲染都需要手动计算宽高比
void update_mvp_matrix() {
    float aspect_ratio = ...; // 从 viewport 获取
    glm::mat4 proj = camera_.get_projection_matrix(..., aspect_ratio);
}
```

**影响**:

- 代码重复：每个使用相机的地方都要计算宽高比
- 容易出错：忘记更新宽高比会导致画面变形
- 职责分散：相机应该自主管理与其相关的视窗参数

**建议方案**:

```cpp
// 方案 A: Camera 持有 Viewport 引用
class Camera {
    std::weak_ptr<SceneViewport> viewport_;
    
    glm::mat4 get_projection_matrix() const {
        if (auto vp = viewport_.lock()) {
            float aspect = vp->aspect_ratio();
            return compute_projection(aspect);
        }
    }
};

// 方案 B: Viewport 主动推送尺寸变化 (推荐)
class SceneViewport {
    void on_resize(std::function<void(float aspect_ratio)> callback);
};

class Camera {
    void set_aspect_ratio(float ratio) { aspect_ratio_ = ratio; }
};
```

#### 🔴 缺陷 2: Vulkan 特殊处理污染 Camera

**问题**:

```cpp
// Camera.hpp - 业务逻辑层包含 Vulkan 细节
class OrbitCamera {
    glm::mat4 get_projection_matrix(...) const {
        glm::mat4 proj = glm::perspective(...);
        proj[1][1] *= -1; // ❌ Vulkan Y轴翻转
        return proj;
    }
};
```

**问题**:

- **平台相关代码污染**: Camera 应该是图形 API 无关的
- **可移植性降低**: 切换到其他图形 API 需要修改 Camera
- **违反分层原则**: Core 层不应该知道 Vulkan 的存在

**建议方案**:

```cpp
// 方案: 渲染后端处理坐标系转换
// Camera.hpp - 纯业务逻辑
class OrbitCamera {
    glm::mat4 get_projection_matrix(...) const {
        return glm::perspective(glm::radians(fov), aspect_ratio, 
                                near_plane, far_plane);
        // 不处理 Y轴翻转
    }
};

// Vulkan 后端统一处理
class VulkanRenderer {
    void set_projection_matrix(const glm::mat4& proj) {
        glm::mat4 vulkan_proj = proj;
        vulkan_proj[1][1] *= -1; // ✅ 在渲染后端处理
        // 或者使用 VK_KHR_maintenance1 扩展
    }
};
```

#### 🔴 缺陷 3: SceneViewport 职责过重

**当前职责**:

```cpp
class SceneViewport {
    // 1. 渲染目标管理
    VkImage color_image_, depth_image_;
    VkDeviceMemory color_memory_, depth_memory_;
    
    // 2. 渲染状态管理
    VkRenderPass render_pass_;
    VkFramebuffer framebuffer_;
    
    // 3. 尺寸管理
    uint32_t width_, height_;           // 渲染目标尺寸
    uint32_t display_width_, display_height_; // 显示尺寸
    
    // 4. ImGui 集成
    VkDescriptorSet imgui_descriptor_set_;
    VkSampler imgui_sampler_;
    
    // 5. 延迟 resize
    void request_resize(uint32_t width, uint32_t height);
    void apply_pending_resize();
    
    // 6. 渲染命令
    void begin_render_pass(VkCommandBuffer cmd);
    void end_render_pass(VkCommandBuffer cmd);
};
```

**问题**:

- **单一职责原则违反**: 一个类管理了渲染目标、渲染状态、UI集成、尺寸管理
- **可测试性降低**: 难以单独测试某个职责
- **复用性降低**: 无法单独使用渲染目标管理功能

**建议重构**:

```cpp
// 1. 渲染目标管理 (RenderTarget)
class RenderTarget {
    VkImage color_image_, depth_image_;
    VkImageView color_view_, depth_view_;
    uint32_t width_, height_;
    VkFormat color_format_, depth_format_;
};

// 2. 视窗控制器 (Viewport)
class Viewport {
    std::shared_ptr<RenderTarget> render_target_;
    
    // 只负责视窗逻辑
    float aspect_ratio() const;
    void on_resize(uint32_t width, uint32_t height);
    
    // 视窗尺寸 vs 渲染目标尺寸
    uint32_t display_width_, display_height_;
    uint32_t render_width_, render_height_;
};

// 3. 渲染通道管理 (RenderPassManager)
class RenderPassManager {
    VkRenderPass render_pass_;
    std::vector<VkFramebuffer> framebuffers_;
    
    void begin(VkCommandBuffer cmd, const RenderTarget& target);
    void end(VkCommandBuffer cmd);
};

// 4. ImGui 集成 (ImGuiViewportBridge)
class ImGuiViewportBridge {
    VkDescriptorSet descriptor_set_;
    VkSampler sampler_;
    
    ImTextureID get_texture_id(const RenderTarget& target);
};
```

---

## 3. 输入处理职责分裂

### 3.1 当前设计

```cpp
// main.cpp - EditorApplication
class EditorApplication {
    void on_update(float delta_time) {
        // ❌ 相机控制在应用层
        if (editor_->is_viewport_hovered() && input_manager()) {
            if (input_manager()->is_mouse_button_pressed(platform::MouseButton::Left)) {
                auto [delta_x, delta_y] = input_manager()->mouse_delta();
                camera_.on_mouse_drag(delta_x, delta_y); // ❌ 直接控制相机
            }
            
            double scroll = input_manager()->scroll_delta();
            camera_.on_mouse_scroll(scroll); // ❌ 直接控制相机
        }
    }
};
```

### 3.2 问题分析

| 问题         | 说明                      | 影响          |
|------------|-------------------------|-------------|
| **控制器缺失**  | 没有 CameraController 中间层 | 相机逻辑与应用逻辑耦合 |
| **输入处理分散** | 部分在 Editor，部分在 main     | 难以维护和扩展     |
| **相机无法自主** | 相机完全被动，没有输入响应能力         | 复用性降低       |

### 3.3 建议重构

```cpp
// 相机控制器 - 解耦输入与相机
class OrbitCameraController {
public:
    void attach_camera(std::shared_ptr<OrbitCamera> camera);
    void attach_viewport(std::shared_ptr<SceneViewport> viewport);
    
    // 输入处理
    void on_mouse_move(float delta_x, float delta_y);
    void on_mouse_scroll(float delta);
    void on_mouse_drag(float delta_x, float delta_y);
    
    // 启用/禁用控制
    void set_enabled(bool enabled) { enabled_ = enabled; }
    bool is_enabled() const { return enabled_; }
    
    // 检查视窗交互
    void update(float delta_time);
    
private:
    std::weak_ptr<OrbitCamera> camera_;
    std::weak_ptr<SceneViewport> viewport_;
    bool enabled_ = true;
    bool is_dragging_ = false;
};

// Editor 中使用
class Editor {
    std::unique_ptr<OrbitCameraController> camera_controller_;
    
    void initialize(...) {
        camera_controller_ = std::make_unique<OrbitCameraController>();
        camera_controller_->attach_camera(camera_);
        camera_controller_->attach_viewport(viewport_);
    }
    
    void on_update(float delta_time) {
        // 检查视窗状态
        bool viewport_active = is_viewport_hovered();
        camera_controller_->set_enabled(viewport_active);
        camera_controller_->update(delta_time);
    }
};
```

---

## 4. Render Graph 与视窗集成问题

### 4.1 当前设计缺陷

```cpp
// main.cpp - render_scene() 方法完全绕过 Render Graph
VkCommandBuffer record_scene_command_buffer(uint32_t frame_index) {
    // ❌ 手动开始 render pass
    viewport_->begin_render_pass(cmd.handle());
    
    // ❌ Render Graph 被当作简单的 pass 容器
    rendering::RenderContext ctx;
    ctx.render_pass = viewport_->render_pass(); // 外部传入
    ctx.framebuffer = viewport_->framebuffer(); // 外部传入
    render_graph_.execute(cmd, ctx);
    
    // ❌ 手动结束 render pass
    viewport_->end_render_pass(cmd.handle());
}

// CubeRenderPass 直接操作命令缓冲
class CubeRenderPass {
    void execute(CommandBuffer& cmd) {
        // ❌ 假设 render pass 已经开始
        // 无法自主管理资源和依赖
    }
};
```

### 4.2 架构问题

```
当前流程：
main.cpp 
  -> 创建 RenderContext (包含 viewport 的 render_pass/framebuffer)
  -> RenderGraph::execute(ctx)
     -> CubeRenderPass::execute(cmd)
        -> 直接渲染（假设 render pass 已绑定）

问题：
1. Render Graph 无法控制 render pass 生命周期
2. Render Graph 无法优化 viewport 的 begin/end
3. Render Graph 无法管理 viewport 资源的依赖
```

### 4.3 建议重构

```cpp
// Render Graph 应该完全控制渲染流程
class RenderGraph {
    // Render Graph 创建和管理 render pass
    void add_viewport_pass(std::shared_ptr<SceneViewport> viewport) {
        auto pass = std::make_unique<ViewportRenderPass>(viewport);
        add_pass(std::move(pass));
    }
    
    // Render Graph 编译时创建 framebuffer 和 render pass
    void compile() {
        // 根据 viewport 尺寸创建 framebuffer
        // 创建 render pass
        // 设置资源依赖
    }
    
    // 执行时自动管理 render pass
    void execute() {
        for (auto& pass : passes_) {
            if (auto vp_pass = dynamic_cast<ViewportRenderPass*>(pass.get())) {
                // 自动 begin/end render pass
                vp_pass->begin_render_pass(cmd);
                vp_pass->execute(cmd);
                vp_pass->end_render_pass(cmd);
            }
        }
    }
};

// 专门的 Viewport Render Pass
class ViewportRenderPass : public RenderGraphPass {
    std::shared_ptr<SceneViewport> viewport_;
    std::vector<std::unique_ptr<RenderPass>> scene_passes_;
    
public:
    void setup(RenderGraphBuilder& builder) override {
        // 声明输出：渲染到 viewport 的颜色和深度附件
        builder.write_image("viewport_color", viewport_->color_image());
        builder.write_image("viewport_depth", viewport_->depth_image());
    }
    
    void execute(RenderContext& ctx) override {
        // 内部管理 render pass
        begin_render_pass();
        
        // 执行子 pass
        for (auto& pass : scene_passes_) {
            pass->execute(ctx);
        }
        
        end_render_pass();
    }
    
    void add_scene_pass(std::unique_ptr<RenderPass> pass) {
        scene_passes_.push_back(std::move(pass));
    }
};

// 使用方式
void initialize_render_graph() {
    auto viewport_pass = std::make_unique<ViewportRenderPass>(viewport_);
    
    // CubeRenderPass 作为子 pass
    auto cube_pass = std::make_unique<CubeRenderPass>(...);
    viewport_pass->add_scene_pass(std::move(cube_pass));
    
    render_graph_.add_pass(std::move(viewport_pass));
    render_graph_.compile();
}
```

---

## 5. 尺寸管理混乱

### 5.1 当前问题

SceneViewport 管理了**两组尺寸**：

```cpp
class SceneViewport {
    uint32_t width_, height_;              // 渲染目标尺寸
    uint32_t display_width_, display_height_; // 显示尺寸
    
    // 延迟 resize
    uint32_t pending_width_, pending_height_;
    bool resize_pending_;
};
```

**问题**:

1. **命名不清晰**: `extent()` vs `display_extent()` 容易混淆
2. **职责不明确**: 为什么需要两组尺寸？
3. **同步复杂**: 延迟 resize 逻辑增加了复杂度

### 5.2 实际使用场景分析

```cpp
// 场景 1: 投影矩阵计算 (main.cpp)
aspect_ratio = display_extent.width / display_extent.height;
// 使用 display_extent - 逻辑正确，确保投影与显示一致

// 场景 2: Render Pass 设置 (SceneViewport.cpp)
render_area.extent = {width_, height_};
// 使用 width_/height_ - 渲染目标尺寸

// 场景 3: Viewport/Scissor 设置 (SceneViewport.cpp)
viewport.width = width_;
scissor.extent = {width_, height_};
// 使用 width_/height_
```

**问题**: 如果 `width_ != display_width_`，投影矩阵与渲染目标不匹配！

### 5.3 建议简化

```cpp
class SceneViewport {
public:
    // 统一尺寸概念
    struct ViewportSize {
        uint32_t render_width, render_height;   // 渲染目标尺寸（可能是高分辨率）
        uint32_t display_width, display_height; // 显示尺寸（逻辑尺寸）
        
        float aspect_ratio() const {
            return float(display_width) / float(display_height);
        }
        
        bool operator!=(const ViewportSize& other) const { ... }
    };
    
    // 一次性获取所有尺寸信息
    ViewportSize get_size() const { return current_size_; }
    
    // Resize 策略枚举
    enum class ResizePolicy {
        Immediate,      // 立即重建
        Deferred,       // 延迟到下一帧
        DynamicScaling // 动态缩放（使用固定渲染目标，缩放显示）
    };
    
    void set_resize_policy(ResizePolicy policy);
    
private:
    ViewportSize current_size_;
    ViewportSize pending_size_;
    ResizePolicy resize_policy_ = ResizePolicy::Deferred;
};
```

---

## 6. 架构重构建议

### 6.1 目标架构

```
┌─────────────────────────────────────────────────────────────┐
│                      Application                            │
├─────────────────────────────────────────────────────────────┤
│                        Editor                               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐   │
│  │   Camera     │  │  Viewport    │  │CameraController  │   │
│  │              │  │              │  │                  │   │
│  │ - view matrix│  │ - size       │  │ - input handling │   │
│  │ - projection │  │ - aspect     │  │ - orbit logic    │   │
│  │ - position   │  │ - render target│ │ - constraints    │   │
│  └──────┬───────┘  └──────┬───────┘  └──────────────────┘   │
│         │                 │                                  │
│         └────────┬────────┘                                  │
│                  │                                           │
│         ┌────────▼────────┐                                  │
│         │   RenderGraph   │                                  │
│         │                 │                                  │
│         │ - ViewportPass  │◄── manages render pass lifecycle │
│         │   - CubePass    │                                  │
│         │   - UI Pass     │                                  │
│         └─────────────────┘                                  │
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────▼───────────────────────────────┐
│                    Vulkan Backend                           │
│         (RenderTarget, RenderPassManager)                   │
└─────────────────────────────────────────────────────────────┘
```

### 6.2 重构步骤

#### Phase 1: 解耦 Camera 与 Vulkan

1. 移除 Camera 中的 `proj[1][1] *= -1`
2. 在 Vulkan 后端添加坐标系转换

#### Phase 2: 引入 CameraController

1. 创建 `OrbitCameraController` 类
2. 将输入处理从 main.cpp 迁移到 Controller
3. Editor 管理 Controller 生命周期

#### Phase 3: 重构 SceneViewport

1. 分离 `RenderTarget` 类
2. 简化尺寸管理
3. 移除 begin/end render pass 方法

#### Phase 4: 完善 Render Graph 集成

1. 创建 `ViewportRenderPass`
2. Render Graph 管理 render pass 生命周期
3. CubeRenderPass 作为子 pass

---

## 7. 代码示例

### 7.1 Camera 简化

```cpp
// Camera.hpp - 图形 API 无关
class OrbitCamera {
public:
    glm::mat4 get_view_matrix() const;
    
    // 不再处理 Vulkan Y轴翻转
    glm::mat4 get_projection_matrix(float fov_degrees, float aspect_ratio, 
                                     float near_plane, float far_plane) const {
        return glm::perspective(glm::radians(fov_degrees), aspect_ratio, 
                                near_plane, far_plane);
    }
    
    // 添加视窗关联
    void attach_viewport(std::shared_ptr<Viewport> viewport);
    
    // 自动获取宽高比
    glm::mat4 get_projection_matrix(float fov_degrees, float near_plane, 
                                     float far_plane) const {
        float aspect = viewport_ ? viewport_->aspect_ratio() : 16.0f / 9.0f;
        return get_projection_matrix(fov_degrees, aspect, near_plane, far_plane);
    }
    
private:
    std::weak_ptr<Viewport> viewport_;
};
```

### 7.2 CameraController 实现

```cpp
class OrbitCameraController {
public:
    void update(float delta_time) {
        if (!enabled_ || !camera_ || !input_manager_) return;
        
        auto input = input_manager_.lock();
        
        // 旋转
        if (input->is_mouse_button_pressed(MouseButton::Left)) {
            auto [dx, dy] = input->mouse_delta();
            if (auto cam = camera_.lock()) {
                cam->on_mouse_drag(dx * sensitivity_, dy * sensitivity_);
            }
        }
        
        // 缩放
        float scroll = input->scroll_delta();
        if (scroll != 0.0f) {
            if (auto cam = camera_.lock()) {
                cam->on_mouse_scroll(scroll * zoom_speed_);
            }
        }
    }
    
    void set_enabled(bool enabled) { enabled_ = enabled; }
    
private:
    std::weak_ptr<OrbitCamera> camera_;
    std::weak_ptr<InputManager> input_manager_;
    std::weak_ptr<Viewport> viewport_;
    bool enabled_ = true;
    float sensitivity_ = 0.5f;
    float zoom_speed_ = 0.1f;
};
```

### 7.3 Viewport 简化

```cpp
class Viewport {
public:
    struct Size {
        uint32_t render_width, render_height;
        uint32_t display_width, display_height;
        
        float aspect_ratio() const {
            return float(display_width) / float(display_height);
        }
    };
    
    Size get_size() const { return size_; }
    float aspect_ratio() const { return size_.aspect_ratio(); }
    
    // 不再管理 render pass
    std::shared_ptr<RenderTarget> get_render_target() const { 
        return render_target_; 
    }
    
private:
    Size size_;
    std::shared_ptr<RenderTarget> render_target_;
};
```

---

## 8. 总结

### 8.1 关键问题清单

| # | 问题                    | 严重程度 | 建议行动                  |
|---|-----------------------|------|-----------------------|
| 1 | Camera 与视窗解耦导致宽高比管理分散 | 🔴 高 | 添加 Camera-Viewport 关联 |
| 2 | Vulkan Y轴翻转在 Camera 中 | 🔴 高 | 移到 Vulkan 后端          |
| 3 | 缺少 CameraController 层 | 🔴 高 | 添加控制器抽象               |
| 4 | SceneViewport 职责过重    | 🟡 中 | 分离 RenderTarget       |
| 5 | Render Graph 与视窗集成不完整 | 🟡 中 | 创建 ViewportRenderPass |
| 6 | 尺寸管理复杂                | 🟢 低 | 统一尺寸结构体               |

### 8.2 重构优先级

1. **P0 (立即)**: 修复 Vulkan Y轴翻转位置
2. **P1 (本周)**: 引入 CameraController
3. **P2 (本月)**: 重构 SceneViewport 职责
4. **P3 (后续)**: 完善 Render Graph 集成

---

**审查完成**: 2026-03-14  
**建议执行**: 按优先级分阶段重构

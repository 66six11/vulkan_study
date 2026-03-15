# Vulkan Engine 渲染层职责划分审查报告

**审查日期**: 2026-03-14  
**审查范围**: Framebuffer、RenderPass、Pipeline、RenderGraph 职责划分  
**严重程度**: 🔴 高 | 🟡 中 | 🟢 低

---

## 1. 执行摘要

渲染层存在多处**职责划分混乱**的问题：

1. **Framebuffer 管理分散** - SwapChain、SceneViewport、FramebufferPool 各自管理，缺乏统一策略
2. **RenderPass 创建权归属不清** - SwapChain 创建 RenderPass 供外部使用，违反封装原则
3. **Pipeline 与 RenderPass 紧耦合** - Pipeline 创建时即绑定 RenderPass，降低灵活性
4. **Render Graph 被绕过** - 实际渲染流程未真正使用 Render Graph 管理

**架构健康度**: 5.5/10

---

## 2. Framebuffer 管理职责分析

### 2.1 当前设计：三足鼎立

项目中存在 **3 个 Framebuffer 管理者**：

```cpp
// 1. SwapChain 管理 Swap Chain Framebuffer
// SwapChain.hpp - 只提供 ImageView，不直接管理 Framebuffer
class SwapChain {
    std::vector<SwapChainImage> images_;  // 包含 Image + ImageView
    VkRenderPass default_render_pass_;     // 创建但不管理 Framebuffer
};

// 2. FramebufferPool 管理 Swap Chain Framebuffer
class FramebufferPool {
    std::vector<std::unique_ptr<Framebuffer>> framebuffers_;
    void create_for_swap_chain(VkRenderPass render_pass, 
                               const std::vector<VkImageView>& image_views, ...);
};

// 3. SceneViewport 管理 Off-screen Framebuffer
class SceneViewport {
    VkFramebuffer framebuffer_;  // 自己创建、自己管理
    VkRenderPass render_pass_;   // 自己创建 RenderPass
    void create_framebuffer();
};
```

### 2.2 问题分析

#### 🔴 问题 1: FramebufferPool 与 SwapChain 配合混乱

```cpp
// main.cpp
class EditorApplication {
    void create_framebuffers() {
        // ❌ SwapChain 提供 ImageView，FramebufferPool 创建 Framebuffer
        // 职责分散在两个类中
        std::vector<VkImageView> image_views;
        for (uint32_t i = 0; i < swap_chain->image_count(); ++i) {
            image_views.push_back(swap_chain->get_image(i).view);
        }
        
        framebuffer_pool_->create_for_swap_chain(
            swap_chain->default_render_pass(),  // ❌ RenderPass 来自 SwapChain
            image_views,
            width_, height_,
            depth_buffer_->view()
        );
    }
};
```

**问题**:

- **创建步骤分散**: 需要外部代码协调 SwapChain 和 FramebufferPool
- **生命周期不同步**: SwapChain recreate 时 FramebufferPool 需要手动重建
- **信息泄露**: 外部代码需要知道 SwapChain 内部 ImageView 细节

**建议方案**:

```cpp
// 方案: SwapChain 自主管理 Framebuffer
class SwapChain {
public:
    // SwapChain 内部创建和管理 Framebuffer
    Framebuffer* get_framebuffer(uint32_t image_index);
    
    // 支持深度附件
    void setup_depth_attachment(VkImageView depth_view);
    
    // Recreate 时自动重建 Framebuffer
    bool recreate();
    
private:
    std::vector<std::unique_ptr<Framebuffer>> framebuffers_;
    VkImageView depth_view_ = VK_NULL_HANDLE;
};

// 使用简化
void initialize() {
    swap_chain_->setup_depth_attachment(depth_buffer_->view());
    // Framebuffer 自动创建，无需外部干预
}
```

#### 🔴 问题 2: SceneViewport 职责过重

SceneViewport 不仅管理渲染目标，还管理 Framebuffer 和 RenderPass：

```cpp
class SceneViewport {
    // 渲染目标管理
    VkImage color_image_, depth_image_;
    VkDeviceMemory color_memory_, depth_memory_;
    VkImageView color_image_view_, depth_image_view_;
    
    // Framebuffer 管理
    VkFramebuffer framebuffer_;
    void create_framebuffer();
    
    // RenderPass 管理
    VkRenderPass render_pass_;
    void create_render_pass();
    
    // 渲染命令
    void begin_render_pass(VkCommandBuffer cmd);
    void end_render_pass(VkCommandBuffer cmd);
};
```

**违反单一职责原则** (SRP)

**建议重构**:

```cpp
// 1. 渲染目标管理 (RenderTarget)
class RenderTarget {
    VkImage color_image_, depth_image_;
    VkImageView color_view_, depth_view_;
    VkDeviceMemory color_memory_, depth_memory_;
    uint32_t width_, height_;
};

// 2. Framebuffer 管理 (FramebufferManager)
class FramebufferManager {
    VkFramebuffer framebuffer_;
    void create(const RenderTarget& target, VkRenderPass render_pass);
};

// 3. 视窗逻辑 (Viewport)
class Viewport {
    std::shared_ptr<RenderTarget> render_target_;
    std::shared_ptr<FramebufferManager> framebuffer_;
    
    // 只保留视窗逻辑
    float aspect_ratio() const;
    void resize(uint32_t width, uint32_t height);
};
```

### 2.3 Framebuffer 生命周期管理

| 类型            | 当前管理者           | 问题      | 建议                    |
|---------------|-----------------|---------|-----------------------|
| Swap Chain FB | FramebufferPool | 需外部协调创建 | 由 SwapChain 内部管理      |
| Viewport FB   | SceneViewport   | 与渲染目标耦合 | 分离 FramebufferManager |
| 临时 FB         | 无               | 无统一策略   | 添加 FramebufferCache   |

---

## 3. RenderPass 管理职责分析

### 3.1 当前设计问题

**RenderPass 创建分散在 3 个类中**:

```cpp
// 1. SwapChain 创建默认 RenderPass
class SwapChain {
    bool create_default_render_pass();        // 无深度
    bool create_render_pass_with_depth(VkFormat depth_format);  // 有深度
};

// 2. SceneViewport 创建自己的 RenderPass
class SceneViewport {
    void create_render_pass();  // 颜色 + 深度，最终布局 SHADER_READ_ONLY
};

// 3. Material/Pipeline 需要使用 RenderPass，但不拥有
class Material {
    void initialize(VkRenderPass render_pass);  // 外部传入
};
```

### 3.2 问题详解

#### 🔴 问题 1: SwapChain 不应该创建 RenderPass

```cpp
// 当前设计 - SwapChain 创建 RenderPass
class SwapChain {
    VkRenderPass default_render_pass() const { return default_render_pass_; }
};

// 使用 - main.cpp
swap_chain->create_render_pass_with_depth(depth_buffer_->format());
VkRenderPass rp = swap_chain->default_render_pass();
```

**问题**:

- **职责混淆**: SwapChain 应该只管理交换链，不应该知道如何创建 RenderPass
- **灵活性降低**: RenderPass 配置被硬编码在 SwapChain 中
- **测试困难**: 无法单独测试 RenderPass 配置

**建议方案**:

```cpp
// 分离 RenderPass 管理
class RenderPassManager {
public:
    // 预定义 RenderPass 配置
    VkRenderPass get_present_render_pass(VkFormat color_format);
    VkRenderPass get_present_render_pass_with_depth(VkFormat color_format, VkFormat depth_format);
    VkRenderPass get_offscreen_render_pass(VkFormat color_format, VkFormat depth_format);
    VkRenderPass get_shadow_render_pass(VkFormat depth_format);
    
    // 缓存 RenderPass 避免重复创建
    VkRenderPass find_or_create(const RenderPassKey& key);
};

// SwapChain 只提供 Format，不创建 RenderPass
class SwapChain {
public:
    VkFormat format() const { return format_; }
    // 移除 default_render_pass()
};

// 使用
auto render_pass = render_pass_manager->get_present_render_pass_with_depth(
    swap_chain_->format(), 
    depth_buffer_->format()
);
```

#### 🔴 问题 2: RenderPass 与 Pipeline 紧耦合

```cpp
// 当前设计 - PipelineConfig 需要 RenderPass
struct GraphicsPipelineConfig {
    VkRenderPass render_pass = VK_NULL_HANDLE;  // 创建时即绑定
    uint32_t     subpass     = 0;
};

class GraphicsPipeline {
    GraphicsPipeline(std::shared_ptr<DeviceManager> device, 
                     const GraphicsPipelineConfig& config);
};
```

**问题**:

- **无法复用 Pipeline**: 相同 Shader 配置，不同 RenderPass 需要创建多个 Pipeline
- **RenderPass 变化需要重建 Pipeline**: 实际上只需要兼容的 RenderPass

**建议方案**:

```cpp
// 延迟 RenderPass 绑定
class GraphicsPipeline {
public:
    // 创建时不绑定 RenderPass
    GraphicsPipeline(std::shared_ptr<DeviceManager> device,
                     const GraphicsPipelineConfig& config);
    
    // 渲染时动态绑定（使用 Pipeline Library 或动态渲染）
    void bind(VkCommandBuffer cmd, VkRenderPass render_pass);
    
    // 或者使用动态渲染（Vulkan 1.3）
    void bind_dynamic(VkCommandBuffer cmd, const DynamicRenderingInfo& info);
};

// 更好的方案：使用 Pipeline Library (Vulkan 1.3)
class PipelineLibrary {
public:
    // 创建 Pipeline 的各个阶段
    VkPipeline create_vertex_input_state(...);
    VkPipeline create_fragment_shader_state(...);
    VkPipeline create_layout(...);
    
    // 运行时链接
    VkPipeline link_for_render_pass(VkRenderPass render_pass);
};
```

### 3.3 RenderPass 缓存策略

```cpp
// 建议添加 RenderPass 缓存
class RenderPassCache {
public:
    struct Key {
        VkFormat color_format;
        VkFormat depth_format;
        VkSampleCountFlagBits samples;
        bool offscreen;  // 最终布局不同
        
        bool operator==(const Key& other) const { ... }
    };
    
    VkRenderPass get_or_create(const Key& key);
    void cleanup_unused();  // 定期清理
    
private:
    std::unordered_map<Key, VkRenderPass, KeyHash> cache_;
};
```

---

## 4. Render Graph 职责边界

### 4.1 当前设计：Render Graph 被架空

```cpp
// main.cpp - 实际渲染流程
void on_render() {
    // ❌ 手动开始 RenderPass
    viewport_->begin_render_pass(scene_cmd);
    
    // ❌ Render Graph 只作为简单的 Pass 容器
    rendering::RenderContext ctx;
    ctx.render_pass = viewport_->render_pass();  // 外部传入
    ctx.framebuffer = viewport_->framebuffer();  // 外部传入
    render_graph_.execute(cmd, ctx);
    
    // ❌ 手动结束 RenderPass
    viewport_->end_render_pass(scene_cmd);
}

// CubeRenderPass - 假设 RenderPass 已在外部开始
void CubeRenderPass::execute(vulkan::RenderCommandBuffer& cmd, const RenderContext& ctx) {
    // ❌ 再次管理 RenderPass
    cmd.begin_render_pass(ctx.render_pass, ctx.framebuffer, ...);
    // ... 渲染
    cmd.end_render_pass();
}
```

### 4.2 问题分析

| 问题                    | 说明                       | 影响            |
|-----------------------|--------------------------|---------------|
| **双重 RenderPass 管理**  | 外部和 Pass 内部都管理 begin/end | 逻辑混乱，容易出错     |
| **Render Graph 无法优化** | 无法控制 RenderPass 生命周期     | 无法合并、重排序 Pass |
| **资源依赖声明无用**          | setup() 声明的依赖未实际使用       | 无法自动生成屏障      |

### 4.3 建议重构

```cpp
// Render Graph 完全控制渲染流程
class RenderGraph {
public:
    // Pass 只声明资源依赖，不管理 RenderPass
    class RenderPass {
        virtual void declare_resources(ResourceDeclaration& decl) = 0;
        virtual void execute(RenderCommand& cmd) = 0;  // 不处理 begin/end
    };
    
    // Render Graph 编译时创建 Framebuffer 和 RenderPass
    void compile() {
        // 1. 分析依赖，确定执行顺序
        // 2. 合并兼容的 Pass
        // 3. 创建必要的 Framebuffer
        // 4. 生成屏障
    }
    
    // 执行时自动管理 RenderPass
    void execute() {
        for (auto& pass_group : compiled_passes_) {
            // 自动 begin RenderPass
            begin_render_pass(pass_group.render_pass, pass_group.framebuffer);
            
            for (auto& pass : pass_group.passes) {
                // 执行 Pass（不处理 begin/end）
                pass->execute(cmd);
            }
            
            // 自动 end RenderPass
            end_render_pass();
        }
    }
};

// CubeRenderPass 简化
class CubeRenderPass : public RenderGraph::RenderPass {
    void declare_resources(ResourceDeclaration& decl) override {
        decl.write_color(0, "color_output", Format::RGBA8);
        decl.write_depth("depth_output", Format::D32);
    }
    
    void execute(RenderCommand& cmd) override {
        // 只关心渲染逻辑，不管理 RenderPass
        material_->bind(cmd);
        cmd.push_constants(...);
        cmd.draw_indexed(...);
    }
};
```

---

## 5. 资源生命周期管理

### 5.1 当前问题：生命周期不明确

```cpp
// 问题：谁拥有这些资源？
class EditorApplication {
    std::unique_ptr<vulkan::DepthBuffer> depth_buffer_;        // 应用层
    std::unique_ptr<vulkan::FramebufferPool> framebuffer_pool_; // 应用层
    std::unique_ptr<rendering::SceneViewport> viewport_;        // 应用层
    
    // 但 RenderPass 在 SwapChain 中创建
    // Framebuffer 在 FramebufferPool 中创建
    // RenderTarget 在 SceneViewport 中创建
};
```

### 5.2 建议：分层资源管理

```cpp
// 1. Vulkan 层 - 原始资源
namespace vulkan {
    class Image;           // VkImage
    class ImageView;       // VkImageView
    class Framebuffer;     // VkFramebuffer
    class RenderPass;      // VkRenderPass
}

// 2. Rendering 层 - 渲染抽象
namespace rendering {
    class RenderTarget {   // Image + ImageView
        std::shared_ptr<vulkan::Image> color_image_;
        std::shared_ptr<vulkan::Image> depth_image_;
    };
    
    class RenderPassManager {  // RenderPass 缓存
        std::unordered_map<Key, std::shared_ptr<vulkan::RenderPass>> cache_;
    };
}

// 3. Application 层 - 业务逻辑
class EditorApplication {
    std::shared_ptr<rendering::RenderTarget> viewport_target_;
    std::shared_ptr<rendering::RenderPassManager> render_pass_mgr_;
    
    // Framebuffer 由 RenderGraph 内部管理，不直接暴露
};
```

---

## 6. 职责划分重构建议

### 6.1 目标架构

```
┌─────────────────────────────────────────────────────────────────┐
│                         Application                             │
│                      (业务逻辑组合)                              │
├─────────────────────────────────────────────────────────────────┤
│                      Render Graph                               │
│         (Pass调度、依赖分析、RenderPass/Framebuffer管理)          │
├─────────────────────────────────────────────────────────────────┤
│                     Rendering Layer                             │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────────┐   │
│  │ RenderPass   │  │ Framebuffer  │  │ RenderTarget         │   │
│  │ Manager      │  │ Manager      │  │ (Image管理)          │   │
│  │ (缓存策略)    │  │ (按需创建)   │  │                      │   │
│  └──────────────┘  └──────────────┘  └──────────────────────┘   │
├─────────────────────────────────────────────────────────────────┤
│                      Vulkan Backend                             │
│         (Image, Buffer, Pipeline, CommandBuffer)               │
└─────────────────────────────────────────────────────────────────┘
```

### 6.2 重构步骤

#### Phase 1: 分离 RenderPass 管理 (P0)

1. 创建 `RenderPassManager` 类
2. 将 SwapChain 中的 RenderPass 创建逻辑迁移至此
3. SwapChain 只提供 Format 信息

#### Phase 2: 重构 Framebuffer 管理 (P1)

1. SwapChain 内部管理自己的 Framebuffer
2. 创建 `FramebufferManager` 用于非 SwapChain Framebuffer
3. SceneViewport 使用 FramebufferManager

#### Phase 3: 完善 Render Graph (P1)

1. Render Graph 完全控制 RenderPass 生命周期
2. RenderPass 只声明资源，不管理 begin/end
3. 实现自动屏障生成

#### Phase 4: 资源生命周期统一 (P2)

1. 使用 `shared_ptr` 统一管理资源生命周期
2. 添加资源引用计数和自动释放
3. 实现资源热重载

---

## 7. 代码示例

### 7.1 RenderPassManager 实现

```cpp
class RenderPassManager {
public:
    struct Key {
        VkFormat color_format;
        VkFormat depth_format;
        VkSampleCountFlagBits samples;
        uint32_t subpass_count;
        
        bool operator==(const Key& o) const {
            return color_format == o.color_format && 
                   depth_format == o.depth_format &&
                   samples == o.samples &&
                   subpass_count == o.subpass_count;
        }
    };
    
    struct KeyHash {
        size_t operator()(const Key& k) const {
            return std::hash<uint64_t>{}(
                (uint64_t(k.color_format) << 32) | 
                uint64_t(k.depth_format)
            );
        }
    };
    
    VkRenderPass get_or_create(const Key& key) {
        auto it = cache_.find(key);
        if (it != cache_.end()) return it->second;
        
        VkRenderPass rp = create_render_pass(key);
        cache_[key] = rp;
        return rp;
    }
    
private:
    std::unordered_map<Key, VkRenderPass, KeyHash> cache_;
};
```

### 7.2 重构后的 SwapChain

```cpp
class SwapChain {
public:
    // 只提供信息，不创建 RenderPass
    VkFormat format() const { return format_; }
    VkExtent2D extent() const { return extent_; }
    
    // 内部管理 Framebuffer
    Framebuffer* get_framebuffer(uint32_t index);
    
    // Recreate 时自动重建 Framebuffer
    bool recreate();
    
private:
    std::vector<std::unique_ptr<Framebuffer>> framebuffers_;
    VkImageView depth_view_ = VK_NULL_HANDLE;
};
```

---

## 8. 总结

### 8.1 关键问题

| # | 问题                        | 严重程度 | 负责组件                           |
|---|---------------------------|------|--------------------------------|
| 1 | SwapChain 创建 RenderPass   | 🔴 高 | SwapChain                      |
| 2 | Framebuffer 管理分散          | 🔴 高 | FramebufferPool, SceneViewport |
| 3 | Render Graph 被绕过          | 🔴 高 | RenderGraph, main.cpp          |
| 4 | Pipeline 与 RenderPass 紧耦合 | 🟡 中 | GraphicsPipeline               |
| 5 | SceneViewport 职责过重        | 🟡 中 | SceneViewport                  |
| 6 | 资源生命周期不明确                 | 🟡 中 | 全局                             |

### 8.2 立即行动项

1. **创建 RenderPassManager** - 集中管理 RenderPass 创建和缓存
2. **重构 SwapChain** - 内部管理 Framebuffer，不再创建 RenderPass
3. **简化 SceneViewport** - 移除 Framebuffer/RenderPass 管理，专注视窗逻辑
4. **完善 Render Graph** - 完全控制渲染流程，移除外部 RenderPass 管理

---

**审查完成**: 2026-03-14  
**建议**: 按 P0 -> P1 -> P2 优先级分阶段重构

# Vulkan Engine 架构问题审查报告

**审查日期**: 2026-03-15  
**审查依据**: Architecture Expert Agent 指导原则 (.iflow/agents/Architecture Expert.agent.md)  
**审查范围**: 主要架构文件、职责分配、依赖关系  
**审查者**: WorkBuddy (基于Architecture Expert指导)

## 一、架构分层违反问题

### 1.1 严重的反向依赖问题

#### **问题1: RenderGraph 依赖具体Vulkan实现**

**文件**: `include/rendering/render_graph/RenderGraph.hpp`
**问题**: RenderGraph 层直接包含 `vulkan/resources/Framebuffer.hpp`，违反了"Vulkan后端细节绝不暴露到Rendering层以上"的原则。

```cpp
// RenderGraph.hpp 中不应该直接包含Vulkan具体实现
#include "vulkan/resources/Framebuffer.hpp"  // 违规!
```

**影响**: 导致RenderGraph与Vulkan强耦合，无法支持其他图形API。

#### **问题2: 裸指针跨层传播**

**文件**: `src/main.cpp`
**问题**: Application层直接操作裸Vulkan对象，违反分层架构。

```cpp
// main.cpp中的违规代码
VkFramebuffer render_target_framebuffer_ = VK_NULL_HANDLE;  // Application层不应该有VkFramebuffer
vkCreateFramebuffer(...);  // Application层直接调用Vulkan API
```

### 1.2 职责边界混乱

#### **问题3: RenderTarget职责不完整**

**文件**: `include/rendering/resources/RenderTarget.hpp`
**问题**: 注释说"不参与RenderPass/Framebuffer管理"，但逻辑上Framebuffer是ImageViews的容器。

```cpp
// RenderTarget.hpp注释:
// - 管理颜色/深度 Image 和 ImageView
// - 提供尺寸信息
// - 不参与 RenderPass/Framebuffer 管理  // 逻辑矛盾!
```

**矛盾**: 如果RenderTarget管理Image和ImageView，那么它创建的Framebuffer也应该由它管理。

#### **问题4: Viewport职责错位**

**文件**: `include/rendering/Viewport.hpp`
**问题**: Viewport管理ImGui的VkDescriptorSet和VkSampler，这是Vulkan后端职责。

```cpp
class Viewport {
    mutable VkDescriptorSet imgui_descriptor_set_ = VK_NULL_HANDLE;  // 违规: Vulkan后端细节
    mutable VkSampler       imgui_sampler_        = VK_NULL_HANDLE;  // 违规: Vulkan后端细节
};
```

## 二、资源管理问题

### 2.1 RAII使用不一致

#### **问题5: 裸VkFramebuffer与RAII Framebuffer混用**

**对比**:

- ✅ 正确的: `framebuffer_pool_` 使用 `vulkan::Framebuffer` RAII包装器
- ❌ 错误的: `render_target_framebuffer_` 使用裸 `VkFramebuffer`

**违反原则**: "所有资源是否由RAII包装器管理"（架构评审清单第251项）

#### **问题6: 缺少Handle-Based资源系统**

**问题**: 项目没有实现Handle系统，导致资源传递使用裸指针或shared_ptr。

```cpp
// 当前做法: 直接传递shared_ptr
std::shared_ptr<RenderTarget> render_target_;

// 应该使用: Handle系统
Handle<RenderTarget> render_target_handle_;
```

### 2.2 生命周期管理混乱

#### **问题7: DeviceManager共享所有权问题**

**文件**: 多个文件
**问题**: 到处使用 `std::shared_ptr<DeviceManager>`，所有权分散。

```cpp
// RenderTarget.cpp
void RenderTarget::initialize(std::shared_ptr<vulkan::DeviceManager> device, ...)

// Viewport.cpp  
void Viewport::initialize(std::shared_ptr<vulkan::DeviceManager> device, ...)

// 等等...
```

**风险**: DeviceManager生命周期难以追踪，可能导致device_为nullptr时仍尝试使用。

#### **问题8: cleanup_resources()顺序依赖**

**文件**: `src/main.cpp`
**问题**: 手动管理清理顺序，容易出错。

```cpp
void Application::cleanup_resources() {
    // 必须按特定顺序清理
    render_graph_.reset();  // 1. 先清理render graph
    // ... 
    framebuffer_pool_.reset();  // 6. 再清理framebuffer pool
    // ...
}
```

## 三、接口设计问题

### 3.1 缺少抽象接口

#### **问题9: 没有IResourceManager接口**

**问题**: 资源创建直接调用具体实现，违反依赖倒置原则。

**当前**:

```cpp
// 直接依赖具体类
class MaterialLoader {
    std::shared_ptr<vulkan::DeviceManager> device_;
};
```

**应该**:

```cpp
// 通过接口依赖
class MaterialLoader {
    std::shared_ptr<IResourceManager> resource_manager_;
};
```

#### **问题10: RenderPass接口设计缺陷**

**文件**: `include/rendering/render_graph/RenderGraphPass.hpp`
**问题**: RenderContext传递裸Vulkan对象，违反分层架构。

```cpp
struct RenderContext {
    VkRenderPass  render_pass = VK_NULL_HANDLE;   // ❌ 裸Vulkan对象
    VkFramebuffer framebuffer = VK_NULL_HANDLE;   // ❌ 裸Vulkan对象
    std::shared_ptr<vulkan::DeviceManager> device;  // ❌ 具体实现依赖
};
```

**违反原则**:

1. Rendering层直接暴露Vulkan后端细节
2. 没有抽象接口，强耦合具体实现
3. 违反"Vulkan后端细节绝不暴露到Rendering层以上"

#### **问题10a: RenderGraph包含Vulkan具体实现**

**文件**: `include/rendering/render_graph/RenderGraphPass.hpp`
**问题**: RenderGraph层直接包含Vulkan具体实现头文件。

```cpp
#include "vulkan/command/CommandBuffer.hpp"  // ❌ 具体实现
#include "vulkan/pipelines/Pipeline.hpp"     // ❌ 具体实现  
#include "vulkan/resources/Framebuffer.hpp"  // ❌ 具体实现
#include "vulkan/device/SwapChain.hpp"       // ❌ 具体实现
```

**影响**: 导致RenderGraph无法独立于Vulkan后端编译和测试。

### 3.2 参数传递混乱

#### **问题11: 混合使用裸句柄和智能指针**

**不一致性**:

```cpp
// 方式1: 裸Vulkan句柄
VkRenderPass render_target_render_pass_ = VK_NULL_HANDLE;

// 方式2: unique_ptr包装
std::unique_ptr<vulkan::FramebufferPool> framebuffer_pool_;

// 方式3: shared_ptr共享
std::shared_ptr<rendering::RenderTarget> render_target_;
```

## 四、线程安全问题

### 4.1 缺少线程安全设计

#### **问题12: 没有明确的线程安全策略**

**问题**: 代码中没有看到线程安全注解或设计。

**风险点**:

- ResourceManager没有锁保护
- CommandPool没有线程关联性
- 多线程资源加载没有同步机制

#### **问题13: 全局状态访问**

**文件**: 多个文件
**问题**: 使用全局logger但没有线程安全保证。

```cpp
logger::info("...");  // 全局函数，可能不是线程安全的
```

## 五、构建系统问题

### 5.1 硬编码路径

#### **问题14: 绝对路径依赖**

**文件**: 多个文件
**问题**: 使用 `D:/TechArt/Vulkan/` 绝对路径。

```cpp
// 在多个文件中发现硬编码路径
std::string shader_path = "D:/TechArt/Vulkan/shaders/...";
```

**影响**: 项目无法在其他机器上编译运行。

### 5.2 CMake模块化不彻底

#### **问题15: 文件发现方式不一致**

**问题**: 部分使用 `file(GLOB)`，部分手动列举。

```cmake
# 方式1: GLOB自动发现
file(GLOB_RECURSE RENDERING_SOURCES "src/rendering/*.cpp")

# 方式2: 手动列举  
set(APPLICATION_SOURCES
        src/application/app/Application.cpp
        # 手动维护列表
)
```

## 六、具体代码示例分析

### 6.1 main.cpp中的架构违反

```cpp
// Application类中的成员变量 - 严重的架构违反
class Application {
    // Application层不应该直接持有Vulkan对象
    VkRenderPass  render_target_render_pass_ = VK_NULL_HANDLE;  // ❌
    VkFramebuffer render_target_framebuffer_ = VK_NULL_HANDLE;  // ❌
    
    // 应该由Rendering层管理
    std::shared_ptr<rendering::RenderTarget> render_target_;    // ✅
};
```

### 6.2 RenderTarget设计矛盾

```cpp
// RenderTarget.hpp中的设计矛盾
class RenderTarget {
    // 管理Vulkan Image和ImageView
    VkImage        color_image_      = VK_NULL_HANDLE;
    VkImageView    color_image_view_ = VK_NULL_HANDLE;
    
    // 但不管理Framebuffer - 逻辑不完整!
    // 缺少: VkFramebuffer framebuffer_;
    
    // 注释说"不参与RenderPass/Framebuffer管理"
    // 但实际上Framebuffer是ImageViews的容器，应该由RenderTarget管理
};

// main.cpp中的矛盾使用
class EditorApplication {
    // 一方面: 使用shared_ptr管理RenderTarget
    std::shared_ptr<rendering::RenderTarget> render_target_;  // ✅ 抽象管理
    
    // 另一方面: 直接管理RenderTarget的Framebuffer
    VkRenderPass render_target_render_pass_ = VK_NULL_HANDLE;  // ❌ 裸对象管理
    
    // Framebuffer被移除了注释说"由RenderTarget自己管理"
    // 但实际上main.cpp中仍然有创建和销毁Framebuffer的逻辑
};
```

### 6.3 清理顺序的脆弱性和Viewport越权

```cpp
// cleanup_resources()中的脆弱清理顺序
void cleanup_resources() {
    // 这些顺序依赖是隐式的，容易出错
    render_graph_.reset();           // 必须先于framebuffer_pool_
    cube_pass_ = nullptr;            // 必须先于material_loader_
    // ...
    framebuffer_pool_.reset();       // 必须在device有效时
    // ...
    material_loader_.reset();        // 必须在device有效时
}

// Viewport.cpp中的越权管理Vulkan对象
void Viewport::cleanup() {
    if (imgui_sampler_ != VK_NULL_HANDLE && device_) {
        vkDestroySampler(device_->device(), imgui_sampler_, nullptr);  // ❌ Viewport层管理Vulkan对象
        imgui_sampler_ = VK_NULL_HANDLE;
    }
}

// Viewport类持有Vulkan后端对象
class Viewport {
    mutable VkDescriptorSet imgui_descriptor_set_ = VK_NULL_HANDLE;  // ❌ 越权
    mutable VkSampler       imgui_sampler_        = VK_NULL_HANDLE;  // ❌ 越权
};
```

## 七、架构重构建议

### 7.1 立即修复的高优先级问题

1. **移除裸VkFramebuffer**
    - 将 `render_target_framebuffer_` 移到RenderTarget类中
    - 使用 `vulkan::Framebuffer` RAII包装器

2. **修复分层依赖**
    - RenderGraph移除对具体Vulkan实现的依赖
    - 创建抽象接口层

3. **统一资源管理**
    - 实现Handle-Based资源系统
    - 统一使用RAII包装器

### 7.2 中期重构任务

4. **接口抽象化**
    - 定义IResourceManager、IRenderDevice等接口
    - 通过依赖注入替换直接依赖

5. **线程安全设计**
    - 添加线程安全注解
    - 设计资源加载的异步机制

### 7.3 长期架构目标

6. **完整Render Graph实现**
    - 实现声明式资源描述
    - 自动同步插入
    - 资源瞬态化

7. **多后端支持准备**
    - 抽象图形API层
    - 支持Vulkan、D3D12等多后端

## 八、结论

当前Vulkan Engine架构存在严重的职责混乱和分层违反问题，主要体现在：

1. **分层架构被破坏**：Application层直接操作Vulkan对象
2. **资源管理不一致**：混合使用裸指针、shared_ptr、unique_ptr
3. **接口设计缺失**：缺少抽象层，强耦合具体实现
4. **职责分配不合理**：RenderTarget不完整，Viewport越权

**最关键的问题是**: `main.cpp`中的 `render_target_framebuffer_` 违反了所有架构原则，必须立即修复。

**建议优先修复顺序**:

1. 修复裸VkFramebuffer问题（当前验证层错误的根本原因）
2. 统一资源管理策略
3. 建立清晰的接口抽象层
4. 实现完整的Render Graph架构

---
**审查完成时间**: 2026-03-15  
**下一步行动**: 根据此报告制定具体的重构计划
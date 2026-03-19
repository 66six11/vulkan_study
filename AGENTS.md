# Vulkan Engine - Agent 使用指南

本文档为 AI Agent 提供在 Vulkan Engine 项目中工作时所需的完整指南，包括架构设计原则、编码规范、技术标准和工作流程。

---

## 项目概述

**Vulkan Engine** 是一个现代化的 C++20 Vulkan 渲染引擎，采用声明式 Render Graph 架构，专注于类型安全、高性能和可维护性。

- **版本**: 2.2.0
- **语言**: C++20
- **图形 API**: Vulkan 1.3+
- **构建系统**: CMake 3.25+ + Conan 2.0
- **许可证**: MIT
- **当前阶段**: Phase 3 - 编辑器集成与 Viewport 系统

---

## 核心设计原则

### 1. RAII 资源管理

所有 Vulkan 资源必须通过强类型 RAII 包装器管理生命周期：

```cpp
// ✅ 正确：使用 RAII 包装器
class Buffer {
    VkBuffer buffer_;
    VmaAllocation allocation_;
public:
    ~Buffer() { 
        vmaDestroyBuffer(allocator_, buffer_, allocation_); 
    }
    Buffer(const Buffer&) = delete;
    Buffer(Buffer&& other) noexcept = default;
};

// ❌ 错误：裸指针管理
VkBuffer* buffer = new VkBuffer;
// ... 忘记 delete
```

### 2. 类型安全

使用 Tag 类型系统防止 Vulkan 对象误用：

```cpp
// ✅ 正确：强类型句柄
template<typename Tag, typename HandleType>
class VulkanHandleBase {
    HandleType handle_;
public:
    constexpr bool valid() const noexcept { 
        return handle_ != VK_NULL_HANDLE; 
    }
};

using Device = VulkanHandleBase<DeviceTag, VkDevice>;
using Queue = VulkanHandleBase<QueueTag, VkQueue>;

// ❌ 错误：直接使用 VkDevice
VkDevice device; // 可能被误用
```

### 3. 声明式渲染

使用 Render Graph 描述渲染流程，自动处理资源和同步：

```cpp
// ✅ 正确：声明式描述
class CubeRenderPass : public RenderPassBase {
    void setup(RenderGraphBuilder& builder) override {
        builder.writeTexture("color", OutputAttachment::Color(0));
        builder.writeTexture("depth", OutputAttachment::Depth());
    }
};

// ❌ 错误：手动管理 RenderPass 依赖
void render() {
    vkCmdBeginRenderPass(...);
    // ... 手动管理屏障和同步
    vkCmdEndRenderPass(...);
}
```

### 4. 现代 C++ 最佳实践

充分利用 C++20 特性：

```cpp
// Concepts 约束
template<typename T>
concept VulkanDevice = requires(T device) {
    { device.instance() } -> std::same_as<Instance>;
    { device.graphics_queue() } -> std::same_as<Queue>;
};

// std::optional 替代空值
std::optional<VkResult> trySubmit();
if (auto result = trySubmit()) {
    // 处理结果
}

// const 正确性
void process(const Image& image) const noexcept;

// noexcept 标记
bool is_valid() const noexcept;
```

### 5. 职责分离

每个类应该有单一、清晰的职责：

```cpp
// ✅ 正确：职责分离
class RenderTarget {
    // 只管理渲染目标（附件、Framebuffer）
    std::vector<VkImageView> attachments_;
    std::shared_ptr<Framebuffer> framebuffer_;
};

class Viewport {
    // 只管理视窗逻辑（尺寸、事件）
    VkExtent2D extent_;
    bool resize_pending_;
};

// ❌ 错误：职责混乱
class Viewport {
    // 既管理视窗又管理 Vulkan 对象
    VkExtent2D extent_;
    VkDescriptorSet descriptor_set_; // 违反职责分离
    VkSampler sampler_;
};
```

---

## 分层架构

### 架构依赖规则

```
┌─────────────────────────────────────────────────────────────┐
│                    Application Layer                        │
│              (应用逻辑、游戏循环、事件处理)                    │
│  ✅ 只使用 Rendering Layer 接口                             │
│  ❌ 禁止直接使用 Vulkan API 或裸 Vulkan 对象                 │
├─────────────────────────────────────────────────────────────┤
│                   Rendering Layer                           │
│         (Render Graph、资源管理、场景管理、材质系统)           │
│  ✅ 只使用 Vulkan Backend 接口                             │
│  ❌ 禁止直接使用 VkRenderPass、VkFramebuffer 等裸对象        │
├─────────────────────────────────────────────────────────────┤
│                   Vulkan Backend                            │
│      (设备管理、管线、资源、同步、命令缓冲管理)                 │
│  ✅ 封装所有 Vulkan API 调用                                │
│  ✅ 提供 RAII 包装器和类型安全接口                           │
├─────────────────────────────────────────────────────────────┤
│                   Platform Layer                            │
│           (窗口管理、输入系统、文件系统)                       │
│  ✅ 提供平台抽象接口                                        │
│  ❌ 禁止包含平台特定代码                                    │
├─────────────────────────────────────────────────────────────┤
│                     Core Layer                              │
│            (数学库、内存管理、工具函数、日志)                   │
│  ✅ 提供基础设施和工具函数                                  │
│  ❌ 不依赖任何其他层                                        │
└─────────────────────────────────────────────────────────────┘
```

### 依赖方向

- **上层可以依赖下层**：Rendering → Vulkan
- **下层不能依赖上层**：Vulkan 不能依赖 Rendering
- **同层之间可以相互依赖**：Rendering 内部模块可以相互依赖

---

## 命名约定

### 命名空间

```cpp
// 命名空间: 小写 + 下划线
namespace vulkan_engine { }

// 子命名空间
namespace vulkan_engine::vulkan { }
namespace vulkan_engine::rendering { }
namespace vulkan_engine::core { }
```

### 类名

```cpp
// PascalCase
class DeviceManager { }
class RenderGraph { }
class RenderPassManager { }
class CameraController { }
```

### 函数名

```cpp
// camelCase
void initialize();
bool is_valid();
void update_frame();
std::shared_ptr<Texture> load_texture();
```

### 成员变量

```cpp
// camelCase + 下划线后缀
class Example {
    uint32_t buffer_size_;
    std::string name_;
    VkDevice device_;
    bool is_valid_;
};
```

### 常量

```cpp
// UPPER_SNAKE_CASE
constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
constexpr uint32_t MAX_DESCRIPTOR_SETS = 1000;
constexpr VkFormat COLOR_FORMAT = VK_FORMAT_B8G8R8A8_SRGB;
```

### 枚举

```cpp
// PascalCase + 描述性名称
enum class KeyAction {
    Press,
    Release,
    Repeat
};

enum class RenderPassType {
    Forward,
    Deferred,
    Compute
};
```

---

## 编码规范

### 文件组织

```cpp
// header guard
#pragma once

// includes: 从系统库到项目库，按字母顺序
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <string>

#include "vulkan/resources/Buffer.hpp"
#include "core/utils/Logger.hpp"

// namespace
namespace vulkan_engine::vulkan {

// 前向声明
class Device;

// 类定义
class DeviceManager {
    // 公共接口
public:
    DeviceManager();
    ~DeviceManager();
    
    // 构造函数
    DeviceManager(const DeviceManager&) = delete;
    DeviceManager& operator=(const DeviceManager&) = delete;
    DeviceManager(DeviceManager&&) noexcept;
    DeviceManager& operator=(DeviceManager&&) noexcept;
    
    // 方法
    void initialize();
    void shutdown();
    
    // 私有实现
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace vulkan_engine::vulkan
```

### 头文件包含顺序

```cpp
// 1. 对应的 C++ 标准库头文件
#include <memory>
#include <vector>
#include <string>

// 2. 第三方库头文件
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

// 3. 项目内部头文件（按字母顺序）
#include "core/utils/Logger.hpp"
#include "vulkan/device/Device.hpp"
#include "vulkan/resources/Buffer.hpp"
```

### const 正确性

```cpp
// ✅ 正确：const 成员函数
class Buffer {
public:
    VkBuffer handle() const noexcept { return buffer_; }
    bool is_valid() const noexcept { return buffer_ != VK_NULL_HANDLE; }
};

// ✅ 正确：const 引用传递
void process(const Image& image);
void update(const std::vector<Mesh>& meshes);

// ❌ 错误：不必要的非 const
VkBuffer handle() { return buffer_; } // 应该是 const
```

### noexcept 标记

```cpp
// ✅ 正确：标记 noexcept
bool is_valid() const noexcept { return handle_ != VK_NULL_HANDLE; }
VkResult submit() noexcept { return vkQueueSubmit(...); }

// ✅ 正确：可能抛出异常的不标记 noexcept
std::shared_ptr<Texture> load_texture(const std::string& path); // 可能抛出 std::runtime_error
```

### 前向声明

优先使用前向声明减少编译依赖：

```cpp
// ✅ 正确：头文件中使用前向声明
class Device;
class Queue;

class DeviceManager {
    std::shared_ptr<Device> device_;  // 使用指针或引用
    // ...
};

// ❌ 错误：包含完整的头文件
#include "Device.hpp"
#include "Queue.hpp"
```

---

## Vulkan 最佳实践

### 资源管理

```cpp
// ✅ 正确：使用 RAII 包装器
class Texture {
    Image image_;
    ImageView view_;
    VmaAllocation allocation_;
    // 析构时自动释放
};

// ✅ 正确：使用智能指针管理共享资源
auto texture = std::make_shared<Texture>(device, path);
material->set_texture("albedo", texture);

// ❌ 错误：手动管理生命周期
VkImage* image = new VkImage;
// ... 忘记 delete
```

### 同步管理

```cpp
// ✅ 正确：使用 FrameSyncManager 的 fence 机制
auto& sync = frame_sync_.frame(current_frame_);
vkWaitForFences(device_, 1, &sync.in_flight_fence, VK_TRUE, UINT64_MAX);
vkResetFences(device_, 1, &sync.in_flight_fence);

// ✅ 正确：使用信号量进行 GPU-GPU 同步
VkSemaphore wait_semaphores[] = { sync.image_available_semaphore() };
VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
VkSemaphore signal_semaphores[] = { sync.render_finished_semaphore() };

// ❌ 错误：避免每帧 vkQueueWaitIdle
vkQueueWaitIdle(graphics_queue_);  // 每帧阻塞，性能极差！

// ❌ 错误：避免 vkDeviceWaitIdle 在热路径中
vkDeviceWaitIdle(device_);  // 阻塞所有队列
```

### 命令缓冲使用

```cpp
// ✅ 正确：每帧录制命令缓冲
auto cmd = command_pool_->allocate_command_buffer();
cmd->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
// ... 录制命令
cmd->end();
queue_->submit({ cmd });

// ✅ 正确：使用多线程录制
std::vector<std::future<CommandBuffer>> futures;
for (auto& thread_data : thread_datas) {
    futures.push_back(std::async(std::launch::async, 
        [&]() { return thread_data.command_pool->allocate(); }));
}
```

### RenderPass 管理

```cpp
// ✅ 正确：使用 RenderPassManager 缓存
auto render_pass_mgr = std::make_shared<RenderPassManager>(device);
VkRenderPass rp = render_pass_mgr->get_present_render_pass_with_depth(
    swap_chain_->format(),
    depth_buffer_->format()
);

// ❌ 错误：每帧创建 RenderPass
VkRenderPassCreateInfo create_info = { /* ... */ };
VkRenderPass render_pass;
vkCreateRenderPass(device, &create_info, nullptr, &render_pass);
// ... 忘记销毁
```

### 资源转换

```cpp
// ✅ 正确：使用 Pipeline Barrier 转换图像布局
VkImageMemoryBarrier barrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    // ...
};
vkCmdPipelineBarrier(cmd, /* ... */, 1, &barrier);

// ❌ 错误：忽略布局转换
// 直接写入 VK_IMAGE_LAYOUT_UNDEFINED 的图像
```

---

## 错误处理

### Vulkan 错误处理

```cpp
// ✅ 正确：检查 Vulkan 返回值
VkResult result = vkQueueSubmit(queue, &submit_info, fence);
if (result != VK_SUCCESS) {
    LOG_ERROR("Failed to submit queue: {}", vulkan::error_string(result));
    return false;
}

// ✅ 正确：使用自定义错误类
#include "vulkan/utils/VulkanError.hpp"

void create_buffer() {
    VkResult result = vkCreateBuffer(device, &create_info, nullptr, &buffer);
    if (result != VK_SUCCESS) {
        throw VulkanError("Failed to create buffer", result);
    }
}
```

### 异常安全

```cpp
// ✅ 正确：RAII 保证异常安全
class DeviceManager {
    std::unique_ptr<Impl> impl_;  // Pimpl 模式
public:
    DeviceManager() {
        impl_ = std::make_unique<Impl>();
        impl_->initialize();  // 构造函数可能抛出
    }
    ~DeviceManager() {
        impl_->shutdown();  // 析构函数不会抛出
    }
};

// ❌ 错误：构造函数中资源泄漏
class DeviceManager {
    VkDevice device_;
    VkQueue queue_;
public:
    DeviceManager() {
        vkCreateDevice(instance, &create_info, nullptr, &device_);
        // 如果这里抛出异常，device_ 不会被销毁！
        get_device_queue(device_, queue_family_index, 0, &queue_);
    }
};
```

---

## 日志和调试

### 日志使用

```cpp
#include "core/utils/Logger.hpp"

// ✅ 正确：使用日志宏
LOG_DEBUG("Creating buffer with size: {}", size);  // 编译期可消除
LOG_INFO("Loading texture: {}", path);
LOG_WARNING("Format not supported, using fallback");
LOG_ERROR("Failed to create buffer: {}", error_string);

// ❌ 错误：直接使用 std::cout 或 printf
std::cout << "Error: " << error << std::endl;  // 不受日志级别控制
```

### 日志级别

```cpp
// 开发环境：使用 Debug 级别
logger::set_level(logger::Level::Debug);

// 发布环境：使用 Warning 级别
logger::set_level(logger::Level::Warning);

// 性能关键代码：只使用 Error 级别
LOG_ERROR("Critical error: {}", error);  // 不会被编译器优化掉
```

### Validation Layers

```cpp
// ✅ 正确：启用验证层
DeviceManager::CreateInfo create_info{
    .enable_validation = true,  // Debug 构建时启用
    .enable_debug_utils = true
};

// ✅ 正确：自定义调试回调
VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* user_data) {
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        LOG_WARNING("Vulkan Validation: {}", data->pMessage);
    }
    return VK_FALSE;
}
```

---

## 性能优化

### 最小化 Draw Calls

```cpp
// ✅ 正确：批量渲染
void batch_render(const std::vector<Mesh>& meshes) {
    for (const auto& mesh : meshes) {
        // 使用 instancing 或间接渲染
        vkCmdDrawIndexed(cmd, mesh.index_count, mesh.instance_count, ...);
    }
}
```

### 命令缓冲复用

```cpp
// ✅ 正确：命令缓冲复用（适合静态场景）
CommandBuffer cb = command_pool->allocate();
cb->begin(VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
// 录制静态场景命令
cb->end();

// 多帧使用同一个命令缓冲
for (uint32_t frame = 0; frame < MAX_FRAMES_IN_FLIGHT; ++frame) {
    queue->submit({ cb }, ...);
}
```

### 多线程录制

```cpp
// ✅ 正确：多线程录制命令缓冲
std::vector<std::thread> threads;
std::vector<CommandBuffer> command_buffers;

for (auto& thread_data : thread_datas) {
    threads.emplace_back([&]() {
        auto cmd = thread_data.command_pool->allocate();
        cmd->begin();
        // 独立录制命令
        cmd->end();
        command_buffers.push_back(cmd);
    });
}

// 等待所有线程完成
for (auto& thread : threads) {
    thread.join();
}

// 提交所有命令缓冲
queue->submit(command_buffers, ...);
```

---

## 测试和验证

### 单元测试

```cpp
// ✅ 正确：使用 Catch2 或 GoogleTest
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Buffer creation", "[vulkan][buffer]") {
    Device device = create_test_device();
    
    Buffer buffer(device, 1024, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    
    REQUIRE(buffer.is_valid());
    REQUIRE(buffer.size() == 1024);
}
```

### 内存安全检查

```cpp
// ✅ 正确：使用 AddressSanitizer
// 构建时添加：
// cmake -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer" ...

// ✅ 正确：使用 Valgrind（Linux）
// valgrind --leak-check=full --show-leak-kinds=all ./vulkan-engine
```

### RenderDoc 捕获

```cpp
// ✅ 正确：启用 RenderDoc 捕获
#ifdef VULKAN_ENGINE_ENABLE_RENDERDOC
    RENDERDOC_API_1_4_1* rdoc_api = renderdoc::get_api();
    if (rdoc_api) {
        rdoc_api->StartFrameCapture(nullptr, nullptr);
        // ... 渲染一帧
        rdoc_api->EndFrameCapture(nullptr, nullptr);
    }
#endif
```

---

## 文档和注释

### 文件头注释

```cpp
/**
 * @file DeviceManager.hpp
 * @brief Vulkan 设备管理器
 * @author Your Name
 * @date 2024-01-01
 * 
 * DeviceManager 负责创建和管理 Vulkan 设备、队列和设备特性。
 * 提供类型安全的 RAII 包装器，自动管理设备生命周期。
 */
```

### 类注释

```cpp
/**
 * @class DeviceManager
 * @brief Vulkan 设备管理器
 * 
 * 封装 Vulkan 设备创建和管理逻辑，包括：
 * - 物理设备选择和评分
 * - 逻辑设备创建和队列获取
 * - 设备特性查询和扩展启用
 * 
 * 使用示例：
 * @code
 * auto device_mgr = std::make_shared<DeviceManager>(create_info);
 * VkDevice device = device_mgr->device().handle();
 * @endcode
 */
class DeviceManager {
    // ...
};
```

### 函数注释

```cpp
/**
 * @brief 创建缓冲区
 * @param size 缓冲区大小（字节）
 * @param usage 缓冲区使用标志
 * @param memory_properties 内存属性（设备本地、主机可见等）
 * @return 创建的缓冲区对象
 * @throws VulkanError 如果创建失败
 * 
 * 创建一个 Vulkan 缓冲区并分配相应的设备内存。
 * 使用 VMA (Vulkan Memory Allocator) 进行内存管理。
 */
Buffer create_buffer(uint64_t size, 
                     VkBufferUsageFlags usage,
                     VkMemoryPropertyFlags memory_properties);
```

### 代码注释

```cpp
// ✅ 正确：解释"为什么"而不是"是什么"
// 使用 double buffering 避免每帧等待 GPU 完成
constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

// ❌ 错误：重复代码
MAX_FRAMES_IN_FLIGHT = 2;  // 设置为 2

// ✅ 正确：复杂逻辑的解释
// 根据 Vulkan 规范，image_available_semaphore 必须等待在
// VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT 阶段
VkPipelineStageFlags wait_stages[] = { 
    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT 
};
```

---

## 常见任务模式

### 创建新类

1. 在 `include/` 创建头文件
2. 在 `src/` 创建实现文件
3. CMake 使用 `file(GLOB_RECURSE)` 自动发现，无需手动修改
4. 遵循 RAII 和类型安全原则
5. 添加适当的注释和文档

### 添加新依赖

1. 在 `conanfile.py` 的 `requires()` 中添加依赖
2. 运行 `conan install . --build=missing`
3. 在 CMake 中使用 `find_package()`
4. 更新 `CMake/Dependencies.cmake`（如果需要）

### 实现 Render Graph Pass

```cpp
#include "rendering/render_graph/RenderPassBase.hpp"

class MyRenderPass : public RenderPassBase {
public:
    void setup(RenderGraphBuilder& builder) override {
        // 声明输入/输出资源
        builder.readTexture("input_tex");
        builder.writeTexture("output_tex", OutputAttachment::Color(0));
    }
    
    void execute(VkCommandBuffer cmd, const RenderContext& context) override {
        // 渲染逻辑
    }
};
```

### 添加着色器

1. 在 `shaders/` 目录创建 `.slang` 文件
2. 运行 `shaders/compile.bat` 编译着色器
3. 在 `ShaderManager` 中加载着色器模块
4. 创建 Pipeline 并绑定着色器

---

## 工作流程

### 修复 Bug

1. **定位问题**：使用日志、Validation Layers、RenderDoc
2. **搜索相关代码**：使用 `search_content` 查找相关函数和类
3. **理解上下文**：阅读相关类的定义和用法
4. **修复问题**：遵循编码规范和最佳实践
5. **测试验证**：编译运行，使用 Valgrind/RenderDoc 验证
6. **更新文档**：如果有必要，更新相关注释和文档

### 添加新功能

1. **设计接口**：遵循分层架构和职责分离原则
2. **实现功能**：使用 RAII、类型安全、现代 C++
3. **编写测试**：确保功能正确性
4. **性能优化**：使用工具分析性能瓶颈
5. **文档更新**：添加必要的注释和使用示例
6. **代码审查**：确保符合项目标准

### 重构代码

1. **理解现状**：阅读相关代码和架构文档
2. **识别问题**：识别违反设计原则的地方
3. **设计重构**：遵循架构设计原则
4. **逐步重构**：小步快跑，保持编译通过
5. **测试验证**：确保重构后功能不变
6. **更新文档**：反映新的架构设计

---

## 重要注意事项

### 禁止事项

- ❌ **禁止**在 Application 层直接使用 Vulkan API
- ❌ **禁止**在 Rendering 层使用裸 VkRenderPass、VkFramebuffer 等对象
- ❌ **禁止**手动管理 Vulkan 资源生命周期（必须使用 RAII）
- ❌ **禁止**在热路径中使用 `vkQueueWaitIdle` 或 `vkDeviceWaitIdle`
- ❌ **禁止**忽略 Vulkan 返回值
- ❌ **禁止**在构造函数中分配资源且不使用 RAII
- ❌ **禁止**硬编码绝对路径
- ❌ **禁止**在头文件中包含不必要的大文件

### 推荐做法

- ✅ **推荐**使用 RAII 包装器管理所有 Vulkan 资源
- ✅ **推荐**使用 RenderPassManager 缓存 RenderPass
- ✅ **推荐**使用 FrameSyncManager 管理每帧同步对象
- ✅ **推荐**使用多线程录制命令缓冲
- ✅ **推荐**使用日志系统而非 std::cout
- ✅ **推荐**启用 Validation Layers（Debug 构建）
- ✅ **推荐**使用 RenderDoc 调试渲染问题
- ✅ **推荐**保持 const 正确性和 noexcept 标记

---

## 参考资源

### 官方文档

- [Vulkan Specification](https://www.khronos.org/registry/vulkan/)
- [Vulkan Guide](https://github.com/KhronosGroup/Vulkan-Guide)
- [Vulkan Best Practices](https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples)

### 项目文档

- `docs/CONAN_BUILD_GUIDE.md` - Conan 构建指南
- `docs/AGENT_GUIDE.md` - Agent 使用指南
- `docs/QUICK_REFERENCE.md` - 快速参考

### 推荐工具

- **RenderDoc** - GPU 调试、帧捕获
- **Tracy** - CPU/GPU 性能分析
- **NSight** - NVIDIA 性能工具
- **Clang-Tidy** - 静态分析
- **AddressSanitizer** - 内存错误检测

---

## 版本历史

- **2.0.0** (2026-03-18) - 完全重写，聚焦于指南和标准
- **1.1.0** (2026-03-15) - 添加架构问题跟踪
- **1.0.0** (2026-03-10) - 初始版本

---

## 联系和贡献

如有问题或建议，请参考项目文档或联系维护者。

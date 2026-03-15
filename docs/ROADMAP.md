# Vulkan Engine — 修复与演进路线图

> **基于**: `docs/code_reviews/` 全项目静态审查（43 份报告，62 个🔴高风险问题）  
> **当前分支**: `2.2`  
> **制定日期**: 2026-03-15  
> **引用文档**: `现代化重构架构方案.md`, `新架构规划.md`, `docs/PHASE3_MIGRATION_FIXES.md`

---

## 一、项目现状评估

### 整体健康度

| 维度        |  评分  | 说明                                                |
|-----------|:----:|---------------------------------------------------|
| **架构设计**  | ⭐⭐⭐⭐ | 分层清晰，Render Graph 框架已立骨，接口设计有前瞻性                  |
| **实现完整度** |  ⭐⭐  | 大量占位符实现（ResourceManager、ShaderManager、Image 等核心类） |
| **运行稳定性** |  ⭐⭐  | 链接错误、每帧 GPU 阻塞、空 barrier 等会导致渲染不正确或崩溃             |
| **工程质量**  | ⭐⭐⭐  | 命名规范一致，RAII 使用正确，但硬编码路径、缺少错误处理                    |
| **性能基线**  |  ⭐⭐  | 主路径存在 `vkQueueWaitIdle` 阻塞，顶点缓冲使用 HOST_VISIBLE    |

### 当前阶段定位

```
[Phase 1 完成] 新目录结构、CMake、基础模块架构
[Phase 2 完成] Render Graph 框架、资源描述类型、Pass 基类
[Phase 3 进行中] 编辑器集成、Viewport resize 回调 ← 当前位置
[Phase 4 未开始] Render Graph 真正执行、资源系统完整实现
[Phase 5 未开始] 高级特性（异步加载、热重载、多线程）
```

---

## 二、问题分级总览

全部 62 个🔴高风险问题按**影响类型**分为四组：

| 类型                  | 数量 | 代表问题                                                                                  |
|---------------------|:--:|---------------------------------------------------------------------------------------|
| **A. 直接导致编译/链接失败**  | 4  | `set_vec2/bool/int` 未实现；`compile_glsl/hlsl` 未实现                                       |
| **B. 运行时崩溃 / 渲染黑屏** | 12 | `Image::upload_data()` 空实现；`transition_layout()` 无实际 barrier；`TimelineSemaphore` 语义错误 |
| **C. 性能击穿**         | 6  | 每帧 `vkQueueWaitIdle`；顶点缓冲 HOST_VISIBLE；每帧大量 logger::info                              |
| **D. 跨机器不可运行**      | 4  | 多处硬编码 `D:/TechArt/Vulkan/` 绝对路径                                                       |

---

## 三、修复路线图

> 每个阶段保证**阶段内可独立编译、运行稳定**后再进入下一阶段。

---

### 🚦 Phase A — 编译修复（预计 0.5 天）

**目标**：消除所有链接错误，项目可正常编译

#### A-1 补全 Material 未实现方法

**文件**: `src/rendering/material/Material.cpp`

```cpp
// 需要补全以下三个函数体
void Material::set_vec2(const std::string& name, const glm::vec2& value) {
    // 临时实现：记录到 params map，bind() 时写入 UBO padding 字段
}
void Material::set_int(const std::string& name, int value) { /* 同上 */ }
void Material::set_bool(const std::string& name, bool value) { /* 同上 */ }
```

#### A-2 处理 ShaderManager 的链接错误

**文件**: `src/rendering/shaders/ShaderManager.cpp`

两种选择（二选一）：注意：当前应该solang

- **Option A（推荐）**：删除头文件中 `compile_glsl` / `compile_hlsl` 的声明，待 Phase C 真正接入 shaderc 时再加回
- **Option B**：添加返回错误状态的空桩实现，明确标注为 `TODO`

#### A-3 消除 ShaderType 重复定义

**文件**: 新建 `include/rendering/RenderingCommonTypes.hpp`，将 `ShaderType` 枚举迁移至此；删除 `ShaderManager.hpp` 和
`ResourceManager.hpp` 中的重复定义

---

### 🚦 Phase B — 渲染正确性修复（预计 2-3 天）

**目标**：Vulkan Validation Layer 零错误，渲染画面正确

#### B-1 修复 Image::transition_layout()

**文件**: `src/vulkan/resources/Image.cpp`

当前问题：只更新软件状态，不发射 barrier。

```cpp
// 修复方案：重新设计接口，区分两种场景
// 场景1：在已有 CommandBuffer 内执行 barrier（同步）
void Image::transition_layout(VkCommandBuffer cmd, VkImageLayout new_layout);

// 场景2：只更新追踪状态（重命名，明确语义）
void Image::set_tracked_layout(VkImageLayout new_layout) {
    current_layout_ = new_layout;
}
```

barrier 参数参考 `CommandBuffer::transition_image_layout()` 中的逻辑，统一封装为内部工具函数。

#### B-2 实现 Image::upload_data()

**文件**: `src/vulkan/resources/Image.cpp`

基本流程（参考现有 `TextureLoader.cpp` 的 staging 逻辑）：

1. 创建 staging buffer（HOST_VISIBLE | HOST_COHERENT）
2. `memcpy` 数据到 staging buffer
3. 提交一次性 command buffer：`vkCmdCopyBufferToImage`
4. Barrier 转换布局为 `SHADER_READ_ONLY_OPTIMAL`
5. 等待完成（短期可用 `vkQueueWaitIdle`，Phase E 升级为 fence + async）

#### B-3 修复 TimelineSemaphore

**文件**: `src/vulkan/sync/Synchronization.cpp`

`TimelineSemaphore` 使用 `VkSemaphoreTypeCreateInfo` 指定 `type = VK_SEMAPHORE_TYPE_TIMELINE`：

```cpp
VkSemaphoreTypeCreateInfo type_info{};
type_info.sType         = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
type_info.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
type_info.initialValue  = initial_value;

VkSemaphoreCreateInfo create_info{};
create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
create_info.pNext = &type_info;
```

#### B-4 修复 ViewportRenderPass::cleanup_framebuffer()

**文件**: `src/rendering/render_graph/ViewportRenderPass.cpp`

当前问题：`cleanup_framebuffer()` 同时销毁了 `render_pass_`，导致 resize 后再 `begin_render_pass()` 使用已销毁的 render
pass 崩溃。

```cpp
void ViewportRenderPass::cleanup_framebuffer() {
    // 只销毁 framebuffer，不销毁 render_pass
    if (framebuffer_ != VK_NULL_HANDLE) {
        vkDestroyFramebuffer(device_, framebuffer_, nullptr);
        framebuffer_ = VK_NULL_HANDLE;
    }
    // render_pass_ 在 cleanup() 中统一销毁
}
```

#### B-5 修复 SceneViewport 描述符集泄漏 注意：SceneViewport需要弃用

**文件**: `src/rendering/SceneViewport.cpp`

resize 前调用 `ImGui_ImplVulkan_RemoveTexture(imgui_descriptor_set_)` 释放旧描述符集，再创建新的：

```cpp
void SceneViewport::resize(uint32_t width, uint32_t height) {
    // 先释放旧描述符集
    if (imgui_descriptor_set_ != VK_NULL_HANDLE) {
        ImGui_ImplVulkan_RemoveTexture(imgui_descriptor_set_);
        imgui_descriptor_set_ = VK_NULL_HANDLE;
    }
    // ... 重建 render target ...
    // 再创建新描述符集
    imgui_descriptor_set_ = ImGui_ImplVulkan_AddTexture(...);
}
```

#### B-6 修复 Buffer::copy_from() 不支持 device-local

**文件**: `src/vulkan/resources/Buffer.cpp`

添加检查：若目标 buffer 为 device-local，自动走 staging 路径：

```cpp
void Buffer::copy_from(const void* data, VkDeviceSize size) {
    if (usage_flags_ & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
        copy_via_staging(data, size);  // 新增 staging 路径
    } else {
        map_and_copy(data, size);      // 原有路径
    }
}
```

#### B-7 修复 DepthBuffer 缺少 SAMPLED_BIT

**文件**: `src/vulkan/resources/DepthBuffer.cpp`

若 DepthBuffer 需要在着色器中采样（例如 shadow map、SSAO），创建时需要加上 `VK_IMAGE_USAGE_SAMPLED_BIT`：

```cpp
image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT 
                 | VK_IMAGE_USAGE_SAMPLED_BIT;  // 添加此标志
```

---

### 🚦 Phase C — 资源系统完整实现（预计 3-5 天）

**目标**：`ResourceManager` 全部接口可用，网格/纹理/材质真正加载到 GPU

#### C-1 ResourceManager 接入真实加载器

**文件**: `src/rendering/resources/ResourceManager.cpp`

| 接口                    | 当前状态           | 修复方案                                      |
|-----------------------|----------------|-------------------------------------------|
| `load_mesh(path)`     | 占位符，返回空 Handle | 调用 `ObjLoader::load()` + `Mesh::upload()` |
| `load_texture(path)`  | 占位符            | 调用 `TextureLoader::load()`                |
| `load_material(path)` | 占位符            | 调用 `MaterialLoader::load()`               |
| `async_requests_` 队列  | 入队但从不消费        | 主循环中每帧调用 `process_async_queue(n)`         |

#### C-2 消除硬编码绝对路径

**涉及文件**: `src/main.cpp`, `src/rendering/material/MaterialLoader.cpp`, `src/rendering/resources/TextureLoader.cpp`

统一使用项目根目录相对路径：

```cpp
// 方案：在 Config 中添加 asset_root_dir
// 默认值通过 CMake 定义 ASSET_ROOT_DIR = CMAKE_SOURCE_DIR
// 运行时通过 argv[0] 推导，或读取配置文件

// MaterialLoader.cpp 修复
std::string resolve_path(const std::string& relative_path) {
    return (Config::instance().asset_root() / relative_path).string();
}
```

#### C-3 ShaderManager 接入 SPIR-V 编译

**文件**: `src/rendering/shaders/ShaderManager.cpp`

两条路径：

- **短期**：`load_shader()` 直接读取预编译的 `.spv` 文件（项目已有 `shaders/*.spv`），绕过 `compile_shader()`
- **中期**：集成 `shaderc`（已是 LunarG SDK 的一部分），实现运行时 GLSL→SPIR-V 编译，支持热重载

```cpp
// 短期修复：直接加载 .spv
std::vector<uint32_t> ShaderManager::load_spirv(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    // ... 读取 spv 字节码 ...
}
```

#### C-4 Mesh 顶点缓冲迁移到 DEVICE_LOCAL

**文件**: `src/rendering/resources/Mesh.cpp`

```cpp
// 当前：HOST_VISIBLE（CPU 可直接写，但 GPU 访问慢）
// 目标：DEVICE_LOCAL + staging buffer（GPU 访问最优）

void Mesh::upload(const std::vector<Vertex>& vertices, 
                  const std::vector<uint32_t>& indices) {
    // 1. 创建临时 staging buffer（HOST_VISIBLE）
    // 2. memcpy 数据到 staging buffer
    // 3. 一次性 command buffer 执行 vkCmdCopyBuffer
    // 4. 销毁 staging buffer（RAII 自动处理）
}
```

---

### 🚦 Phase D — 性能修复（预计 1-2 天）

**目标**：CPU/GPU 流水线不被阻塞，每帧日志可控

#### D-1 移除主循环中的 vkQueueWaitIdle

**文件**: `src/main.cpp`

当前问题：每帧末尾调用 `vkQueueWaitIdle()` 强制 CPU 等待 GPU 完成，彻底破坏 CPU/GPU 并行。

修复方案：

```cpp
// 使用 FrameSyncManager 的 fence 机制替代
// 在帧开始时等待上上帧（frame_index - MAX_FRAMES_IN_FLIGHT）的 fence
// 而不是在帧末等待当帧完成

auto& sync = frame_sync_.frame(current_frame_);
vkWaitForFences(device_, 1, &sync.in_flight_fence, VK_TRUE, UINT64_MAX);
vkResetFences(device_, 1, &sync.in_flight_fence);
```

#### D-2 Logger 添加日志级别过滤

**文件**: `include/core/utils/Logger.hpp`

```cpp
enum class LogLevel { Debug, Info, Warning, Error, Off };

// 全局日志级别（Release 默认 Warning，Debug 默认 Info）
inline LogLevel g_log_level = LogLevel::Info;

#define LOG_INFO(msg)    if (vulkan_engine::LogLevel::Info    >= g_log_level) logger::info(msg)
#define LOG_WARNING(msg) if (vulkan_engine::LogLevel::Warning >= g_log_level) logger::warn(msg)
```

#### D-3 移除热路径中的 logger::info

**涉及文件**: `src/rendering/render_graph/CubeRenderPass.cpp`, `src/main.cpp`

每帧调用的函数（`execute()`、`render()`、主循环体）中的 `logger::info` 改为 `LOG_DEBUG` 或删除。

---

### 🚦 Phase E — Render Graph 完整化（预计 5-7 天）

**目标**：Render Graph 真正驱动渲染，Pass 间资源 barrier 自动管理

#### E-1 RenderGraph 添加拓扑排序

**文件**: `src/rendering/render_graph/RenderGraph.cpp`

```cpp
void RenderGraph::compile() {
    // 1. 构建 DAG：节点 = Pass，边 = 资源读写依赖
    // 2. Kahn 算法拓扑排序（BFS）
    // 3. 检测循环依赖（报错）
    // 4. 生成执行顺序 execution_order_
    
    // 简单实现：
    std::vector<RenderGraphNode*> sorted;
    // ... BFS 拓扑排序 ...
    execution_order_ = std::move(sorted);
}
```

#### E-2 RenderGraphResource 实现 barrier 发射

**文件**: `src/rendering/render_graph/RenderGraphResource.cpp`

外部资源（`external = true`）的 barrier 当前被直接跳过，需要补全：

```cpp
void RenderGraphResource::emit_barrier(VkCommandBuffer cmd, 
                                        VkImageLayout new_layout) {
    if (image_handle == VK_NULL_HANDLE) return;
    
    VkImageMemoryBarrier barrier{};
    barrier.oldLayout = current_layout;
    barrier.newLayout = new_layout;
    // ... 填充 src/dst stage mask 和 access mask ...
    
    vkCmdPipelineBarrier(cmd, src_stage, dst_stage, 
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
    current_layout = new_layout;
}
```

#### E-3 GeometryRenderPass 完善 begin/end render pass

**文件**: `src/rendering/render_graph/RenderGraphPass.cpp`

`GeometryRenderPass::execute()` 缺少 `vkCmdBeginRenderPass` / `vkCmdEndRenderPass` 调用，几何体无法实际渲染：

```cpp
void GeometryRenderPass::execute(VkCommandBuffer cmd, const RenderContext& ctx) {
    VkRenderPassBeginInfo begin_info{};
    begin_info.renderPass  = render_pass_;
    begin_info.framebuffer = framebuffer_;
    begin_info.renderArea  = {{0, 0}, {extent_.width, extent_.height}};
    // ... clear values ...
    
    vkCmdBeginRenderPass(cmd, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
    // 绑定管线、描述符、绘制命令
    vkCmdEndRenderPass(cmd);
}
```

#### E-4 PresentRenderPass::execute() 实现

**文件**: `src/rendering/render_graph/RenderGraphPass.cpp`

当前为空实现，需要将最终颜色附件 blit 到交换链图像（或直接使用交换链 framebuffer）。

#### E-5 RenderGraph next_id 线程安全

**文件**: `src/rendering/render_graph/RenderGraph.cpp`

将 `static` 局部变量改为 `std::atomic<uint32_t>`：

```cpp
static std::atomic<uint32_t> next_id{0};
uint32_t id = next_id.fetch_add(1, std::memory_order_relaxed);
```

---

### 🚦 Phase F — Scene & Camera 系统（预计 3-4 天）

**目标**：Scene 可管理实体生命周期，Camera 控制器正常工作

#### F-1 修复 Scene::destroy_entity() 悬空指针

**文件**: `src/rendering/scene/Scene.cpp`

当前问题：`destroy_entity(Entity*)` 从内部容器删除后，外部持有的裸指针变成悬空指针。

修复方案：迁移到 **Handle-based** 实体管理（参考架构方案中的 `ResourceHandle<T>` 设计）：

```cpp
// 将 Entity* 替换为 EntityHandle（ID + generation）
struct EntityHandle {
    uint32_t id;
    uint32_t generation;
    bool is_valid() const;
};

EntityHandle Scene::create_entity(const std::string& name);
void         Scene::destroy_entity(EntityHandle handle);
Entity*      Scene::get_entity(EntityHandle handle);  // 内部验证 generation
```

#### F-2 实现 Scene::update() 和 Scene::render()

当前两个函数均为空实现，需要：

- `update(delta_time)`：遍历实体，调用组件更新（Transform、Script 等）
- `render(cmd, camera)`：遍历可渲染实体，提交到 RenderGraph

#### F-3 OrbitCameraController delta_time 接入

**文件**: `src/rendering/camera/CameraController.cpp`

`OrbitCameraController::update()` 接收 `delta_time` 参数但完全未使用，导致相机速度与帧率强耦合：

```cpp
void OrbitCameraController::update(float delta_time) {
    // 将所有移动量乘以 delta_time
    orbit_speed_  = base_orbit_speed_  * delta_time;
    pan_speed_    = base_pan_speed_    * delta_time;
    zoom_speed_   = base_zoom_speed_   * delta_time;
}
```

#### F-4 CameraController attach_camera 接受 OrbitCamera*

**文件**: `include/rendering/camera/CameraController.hpp`

当前 `attach_camera` 只接受基类 `Camera*`，但 `OrbitCameraController` 需要调用 `OrbitCamera` 特有接口（`orbit()`, `pan()`,
`zoom()`），需要拓宽接口或引入 Concept：

```cpp
// 方案：重载或模板
template<typename CameraT>
    requires std::derived_from<CameraT, Camera>
void attach_camera(CameraT* camera);
```

---

### 🚦 Phase G — 工程质量与可移植性（持续进行）

**目标**：跨机器可运行，代码可维护性提升

#### G-1 统一路径管理

新建 `include/core/utils/PathUtils.hpp`：

```cpp
namespace path_utils {
    // 返回项目资源根目录（相对 exe 推导，或读取 VULKAN_ENGINE_ASSET_ROOT 环境变量）
    std::filesystem::path asset_root();
    
    // 解析相对于资源根的路径
    std::filesystem::path resolve_asset(std::string_view relative);
    
    // 解析着色器路径
    std::filesystem::path resolve_shader(std::string_view name);
}
```

在 `MaterialLoader`、`TextureLoader`、`main.cpp`、`ShaderManager` 中统一调用此工具。

#### G-2 FileSystem 修复硬编码权限

**文件**: `src/platform/filesystem/FileSystem.cpp`

```cpp
// 修复 is_readable / is_writable
bool FileSystem::is_readable(const std::string& path) const {
    return std::filesystem::exists(path) && 
           (std::filesystem::status(path).permissions() & 
            std::filesystem::perms::owner_read) != std::filesystem::perms::none;
}
```

#### G-3 VK_CHECK 处理 VK_SUBOPTIMAL_KHR

**文件**: `include/vulkan/utils/VulkanError.hpp`

```cpp
#define VK_CHECK(expr) do {                                          \
    VkResult _result = (expr);                                       \
    if (_result != VK_SUCCESS && _result != VK_SUBOPTIMAL_KHR) {    \
        throw VulkanError(_result, #expr);                           \
    }                                                                \
} while(0)
```

#### G-4 Device::supports_feature() 真实检查

**文件**: `src/vulkan/device/Device.cpp`

当前返回硬编码 `true`，需要查询 `VkPhysicalDeviceFeatures`：

```cpp
bool DeviceManager::supports_feature(const std::string& feature_name) const {
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(physical_device_, &features);
    // 用 feature_name 查找对应字段（或改为枚举参数更类型安全）
}
```

#### G-5 SwapChain queue family 动态查询

**文件**: `src/vulkan/device/SwapChain.cpp`

`graphics_queue_family_` 硬编码为 0，应从 `DeviceManager` 查询：

```cpp
graphics_queue_family_ = device_manager_.graphics_queue_family_index();
```

---

## 四、执行优先级矩阵

> 按**投入产出比**排序，越靠前效益越高

|  优先级  | 任务                                   | 预计耗时  | 解锁的功能           |
|:-----:|--------------------------------------|:-----:|-----------------|
| P0 🔥 | Phase A：消除链接错误                       | 0.5 天 | 项目可正常编译         |
| P0 🔥 | Phase B-4：Viewport resize 不崩溃        | 0.5 天 | 编辑器 Viewport 稳定 |
| P0 🔥 | Phase C-2：消除硬编码路径                    | 0.5 天 | 可在其他机器运行        |
| P1 ⚡  | Phase B-1/B-2：Image barrier + upload |  1 天  | 纹理正确显示          |
| P1 ⚡  | Phase C-1：ResourceManager 接入加载器      |  2 天  | 真正加载网格/纹理       |
| P1 ⚡  | Phase C-3：ShaderManager 加载 SPV       | 0.5 天 | 着色器正常使用         |
| P1 ⚡  | Phase D-1：移除 vkQueueWaitIdle         | 0.5 天 | 帧率不被 GPU 阻塞     |
| P2 📈 | Phase C-4：顶点缓冲 DEVICE_LOCAL          |  1 天  | GPU 渲染性能提升      |
| P2 📈 | Phase D-2/D-3：日志级别过滤                 | 0.5 天 | 调试/发布可切换        |
| P2 📈 | Phase E-1/E-2：RenderGraph 拓扑+barrier |  3 天  | RG 自动管理资源同步     |
| P3 🔮 | Phase F：Scene + Camera 系统            | 3-4 天 | 场景管理完整可用        |
| P3 🔮 | Phase E-3/E-4：Pass 完整执行              |  2 天  | RG 驱动完整渲染流程     |
| P4 🧹 | Phase G：工程质量全面提升                     |  持续   | 可维护性、可移植性       |

---

## 五、架构演进方向

基于现有架构与 `现代化重构架构方案.md`，推荐以下演进路径：

### 近期（修复阶段，1-2 周）

```
当前状态: Render Graph 框架 + 大量占位符
目标: 所有核心路径真实实现，零 validation error
```

重点是**填充实现**，不改接口。

### 中期（强化阶段，1 个月）

1. **ResourceManager Handle 化**：将 `Entity*`、`Mesh*`、`Material*` 等裸指针全部换成 `Handle<T>`（ID +
   generation），彻底消除悬空指针问题
2. **异步资源加载**：`async_requests_` 队列真正工作，后台线程处理资源 I/O，主线程只做 GPU 上传
3. **ShaderManager + shaderc**：运行时 GLSL 编译，支持 `#include`，配合热重载
4. **Sampler Cache**：`Material` 中的 Sampler 提取到 `SamplerCache`，相同参数共享

### 远期（现代化阶段，2-3 个月）

1. **VMA 集成**：用 `VulkanMemoryAllocator` 替换手写内存查询，自动内存分配、碎片整理
2. **多线程命令录制**：每个 RenderGraph Pass 在独立线程的 secondary command buffer 上录制
3. **Descriptor Indexing / Bindless**：减少 pipeline 和 descriptor set 切换开销
4. **Tracy 集成**：CPU/GPU 性能剖析，可视化每个 Pass 耗时

---

## 六、各阶段验证标准

| 阶段完成后   | 验证方法                              | 预期结果                        |
|---------|-----------------------------------|-----------------------------|
| Phase A | `cmake --build`、链接                | 零链接错误                       |
| Phase B | 开启 Validation Layer 运行            | 控制台零 `Validation layer:` 错误 |
| Phase C | 加载并显示一个 OBJ + 纹理                  | 网格正确显示，纹理贴图正确               |
| Phase D | 使用 RenderDoc 的 `Frame Statistics` | 每帧提交一次，无额外 Queue 等待         |
| Phase E | 添加第二个 RenderPass 节点               | RG 自动处理两个 Pass 的资源 barrier  |
| Phase F | 在 Scene 中创建/销毁实体                  | 无崩溃，无内存泄漏（AddressSanitizer） |

---

## 七、参考资源

| 资源                                                                                                                                           | 用途                   |
|----------------------------------------------------------------------------------------------------------------------------------------------|----------------------|
| [Vulkan Specification — Pipeline Barriers](https://registry.khronos.org/vulkan/specs/1.3/html/vkspec.html#synchronization-pipeline-barriers) | Phase B barrier 实现参考 |
| [Vulkan Memory Allocator 文档](https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/)                                         | Phase 远期 VMA 集成      |
| [shaderc README](https://github.com/google/shaderc)                                                                                          | Phase C-3 运行时编译      |
| [RenderDoc 文档](https://renderdoc.org/docs/)                                                                                                  | 所有阶段验证工具             |
| [Vulkan Guide — Render Passes](https://vkguide.dev/docs/chapter-1/vulkan_renderpass/)                                                        | Phase E RG 实现参考      |
| [Frostbite Render Graph 论文](https://www.gdcvault.com/play/1024612)                                                                           | RG 设计理论参考            |
| `docs/code_reviews/INDEX.md`                                                                                                                 | 所有问题的完整列表            |
| `docs/PHASE3_MIGRATION_FIXES.md`                                                                                                             | Phase 3 已完成的修复记录     |

---

*本文档由 WorkBuddy 基于全项目代码审查自动生成，建议结合 RenderDoc + Vulkan Validation Layers 运行时验证后持续更新。*

# Vulkan Engine — 代码审查索引

**生成日期**: 2026-03-15
**覆盖范围**: 全部 `.hpp` / `.cpp` 源文件（约 91 个文件，43 份报告）
**项目分支**: `2.2`

---

## 风险等级说明

| 图例 | 含义                            |
|----|-------------------------------|
| 🔴 | 高风险 — 可能导致崩溃、数据损坏、链接错误或严重功能缺失 |
| 🟡 | 中风险 — 逻辑错误、性能问题、API 误用或接口设计缺陷 |
| 🟢 | 低风险 — 代码风格、可维护性、注释或微小语义问题     |

---

## 快速风险概览

> 下表列出每份报告中**最高严重级别问题数量**的汇总，便于优先排序修复工作。

| 报告文件                                     | 覆盖源文件                          | 🔴 高 | 🟡 中 | 🟢 低 | 最严重问题摘要                                                |
|------------------------------------------|--------------------------------|:----:|:----:|:----:|--------------------------------------------------------|
| [main_EditorApplication](#应用入口)          | `src/main.cpp`                 |  4   |  4   |  2   | 每帧 `vkQueueWaitIdle` 阻塞；硬编码绝对路径                        |
| [application_Application](#应用层)          | `Application.hpp/.cpp`         |  1   |  2   |  2   | `on_window_resize` 双重 recreate 风险                      |
| [application_Config](#应用层)               | `Config.hpp/.cpp`              |  0   |  2   |  2   | `is_readable/is_writable` 硬编码 true                     |
| [editor_Editor](#编辑器)                    | `Editor.hpp/.cpp`              |  1   |  2   |  2   | 废弃 `render_scene()` 未清理                                |
| [editor_ImGuiManager](#编辑器)              | `ImGuiManager.hpp/.cpp`        |  1   |  2   |  2   | 材质参数为静态局部变量未与 Material 绑定                              |
| [core_Camera](#核心层)                      | `Camera.hpp`                   |  1   |  1   |  2   | up vector 硬编码，极点附近数值不稳定                                |
| [core_Vector](#核心层)                      | `Vector.hpp`                   |  0   |  2   |  2   | 缺少 `scalar * Vec` 形式；零向量 normalized 未文档化               |
| [core_Logger](#核心层)                      | `Logger.hpp`                   |  1   |  1   |  1   | 无日志级别过滤，每帧大量 info 输出                                   |
| [platform_Window](#平台层)                  | `Window.hpp/.cpp`              |  1   |  2   |  2   | 多窗口场景 glfwInit/Terminate 崩溃                            |
| [platform_InputManager](#平台层)            | `InputManager.hpp/.cpp`        |  1   |  2   |  2   | `map_glfw_key` 只映射 6 个键                                |
| [platform_FileSystem](#平台层)              | `FileSystem.hpp/.cpp`          |  1   |  2   |  2   | 权限检查硬编码；creation_time 语义错误                             |
| [vulkan_Device](#vulkan-后端)              | `Device.hpp/.cpp`              |  2   |  2   |  2   | `supports_feature()` 硬编码 true                          |
| [vulkan_SwapChain](#vulkan-后端)           | `SwapChain.hpp/.cpp`           |  2   |  2   |  2   | `shutdown()` 代码损坏；queue family 硬编码 0                   |
| [vulkan_CommandBuffer](#vulkan-后端)       | `CommandBuffer.hpp/.cpp`       |  1   |  2   |  2   | 未知 layout 转换返回空 barrier                                |
| [vulkan_Synchronization](#vulkan-后端)     | `Synchronization.hpp/.cpp`     |  1   |  2   |  1   | `TimelineSemaphore` 实际创建的是普通 semaphore                 |
| [vulkan_Pipeline](#vulkan-后端)            | `Pipeline.hpp/.cpp`            |  1   |  2   |  2   | 移动后 `layout_` 不清零                                      |
| [vulkan_RenderPassManager](#vulkan-后端)   | `RenderPassManager.hpp/.cpp`   |  1   |  2   |  2   | `cleanup_unused()` 清空所有缓存，无引用计数                        |
| [vulkan_Buffer](#vulkan-后端)              | `Buffer.hpp/.cpp`              |  2   |  2   |  2   | `copy_from()` 不支持 device-local；`map()` 未检查已映射状态        |
| [vulkan_Image](#vulkan-后端)               | `Image.hpp/.cpp`               |  2   |  2   |  1   | `transition_layout()` 不实际执行 barrier；`upload_data()` 为空 |
| [vulkan_DepthBuffer](#vulkan-后端)         | `DepthBuffer.hpp/.cpp`         |  2   |  1   |  2   | 缺少 `SAMPLED_BIT`；`vkBindImageMemory` 返回值未检查            |
| [vulkan_Framebuffer](#vulkan-后端)         | `Framebuffer.hpp/.cpp`         |  1   |  2   |  2   | 不验证 RenderPass 兼容性                                     |
| [vulkan_UniformBuffer](#vulkan-后端)       | `UniformBuffer.hpp`            |  2   |  1   |  2   | 析构时 device 可能已销毁；`frame_count=0` 越界                    |
| [vulkan_ShaderModule](#vulkan-后端)        | `ShaderModule.hpp/.cpp`        |  1   |  2   |  2   | 路径不支持相对搜索                                              |
| [vulkan_VulkanError](#vulkan-后端)         | `VulkanError.hpp`              |  1   |  1   |  1   | `VK_CHECK` 不处理 `VK_SUBOPTIMAL_KHR`                     |
| [vulkan_CoordinateTransform](#vulkan-后端) | `CoordinateTransform.hpp`      |  1   |  1   |  1   | 未处理 Z 深度 [0,1] 映射                                      |
| [rendering_RenderGraph](#渲染层)            | `RenderGraph.hpp/.cpp`         |  2   |  2   |  2   | 无拓扑排序；`next_id` 为 static 局部变量                          |
| [rendering_RenderGraphPass](#渲染层)        | `RenderGraphPass.hpp/.cpp`     |  2   |  2   |  1   | `PresentRenderPass::execute()` 为空；几何 Pass 无 begin/end  |
| [rendering_RenderGraphResource](#渲染层)    | `RenderGraphResource.hpp/.cpp` |  1   |  2   |  2   | 外部资源 barrier 被跳过                                       |
| [rendering_RenderGraphTypes](#渲染层)       | `RenderGraphTypes.hpp`         |  1   |  2   |  1   | Format 枚举过简；跨类型隐式转换破坏类型安全                              |
| [rendering_CubeRenderPass](#渲染层)         | `CubeRenderPass.hpp/.cpp`      |  1   |  2   |  2   | 每帧大量 `logger::info`；`setup()` 未声明资源依赖                  |
| [rendering_ViewportRenderPass](#渲染层)     | `ViewportRenderPass.hpp/.cpp`  |  2   |  2   |  1   | `cleanup_framebuffer()` 同时销毁 render_pass 导致 resize 崩溃  |
| [rendering_Material](#渲染层)               | `Material.hpp/.cpp`            |  2   |  2   |  2   | `set_vec2/set_int/set_bool` 未实现（链接错误）；热路径编译管线          |
| [rendering_MaterialLoader](#渲染层)         | `MaterialLoader.hpp/.cpp`      |  2   |  2   |  1   | `resolve_path()` 硬编码 `D:/TechArt/Vulkan/`              |
| [rendering_Viewport](#渲染层)               | `Viewport.hpp/.cpp`            |  2   |  2   |  2   | `const_cast` 修改 mutable 成员；resize 后旧描述符集未释放            |
| [rendering_RenderTarget](#渲染层)           | `RenderTarget.hpp/.cpp`        |  1   |  2   |  2   | cleanup 中 `vkDeviceWaitIdle` 阻塞；每次 resize 创建临时命令池      |
| [rendering_Mesh](#渲染层)                   | `Mesh.hpp/.cpp`                |  1   |  2   |  2   | 顶点缓冲使用 HOST_VISIBLE，性能损失                               |
| [rendering_ObjLoader](#渲染层)              | `ObjLoader.hpp/.cpp`           |  1   |  2   |  2   | 顶点色彩为 sin 占位；fan 三角化不适合非凸多边形                           |
| [rendering_TextureLoader](#渲染层)          | `TextureLoader.hpp/.cpp`       |  3   |  2   |  1   | 硬编码绝对路径；staging buffer 无 RAII；`vkQueueWaitIdle` 阻塞     |
| [rendering_ResourceManager](#渲染层)        | `ResourceManager.hpp/.cpp`     |  3   |  3   |  2   | 所有 load 为占位符；异步队列从未消费                                  |
| [rendering_Scene](#渲染层)                  | `Scene.hpp/.cpp`               |  3   |  3   |  2   | `destroy_entity` 使指针悬空；update/render 完全为空              |
| [rendering_SceneViewport](#渲染层)          | `SceneViewport.hpp/.cpp`       |  3   |  2   |  2   | ImGui 描述符集泄漏；resize 阻塞 GPU；`const_cast` 滥用             |
| [rendering_ShaderManager](#渲染层)          | `ShaderManager.hpp/.cpp`       |  3   |  3   |  2   | compile 输出空 bytecode；`compile_glsl/hlsl` 链接错误          |
| [rendering_CameraController](#渲染层)       | `CameraController.hpp/.cpp`    |  2   |  3   |  2   | attach_camera 类型过窄；delta_time 未使用                      |

---

## 报告详情目录

### 应用入口

| 报告                                                     | 描述                                              |
|--------------------------------------------------------|-------------------------------------------------|
| [main_EditorApplication.md](main_EditorApplication.md) | `EditorApplication` 顶层组装类，主循环、渲染提交、窗口 resize 处理 |

### 应用层

| 报告                                                       | 描述                                    |
|----------------------------------------------------------|---------------------------------------|
| [application_Application.md](application_Application.md) | `ApplicationBase` 基类，主循环驱动，窗口/设备/输入管理 |
| [application_Config.md](application_Config.md)           | `Config` 配置文件读写，文件系统元数据查询             |

### 编辑器

| 报告                                               | 描述                                             |
|--------------------------------------------------|------------------------------------------------|
| [editor_Editor.md](editor_Editor.md)             | `Editor` 编辑器门面类，ImGui Docking 布局，Viewport 面板管理 |
| [editor_ImGuiManager.md](editor_ImGuiManager.md) | `ImGuiManager` ImGui Vulkan 后端初始化，材质参数 UI 面板   |

### 核心层

| 报告                               | 描述                                   |
|----------------------------------|--------------------------------------|
| [core_Camera.md](core_Camera.md) | `OrbitCamera` 轨道相机，视图/投影矩阵，鼠标拖拽/滚轮输入 |
| [core_Vector.md](core_Vector.md) | `Vec2/Vec3/Vec4` 数学向量类，基础线性代数运算      |
| [core_Logger.md](core_Logger.md) | 全局日志系统，`logger::info/warn/error` 宏包装 |

### 平台层

| 报告                                                   | 描述                                        |
|------------------------------------------------------|-------------------------------------------|
| [platform_Window.md](platform_Window.md)             | `Window` GLFW 窗口封装，事件回调，Vulkan Surface 创建 |
| [platform_InputManager.md](platform_InputManager.md) | `InputManager` 键盘/鼠标状态双缓冲，Pimpl 实现        |
| [platform_FileSystem.md](platform_FileSystem.md)     | `FileSystem` 文件系统工具，路径操作，文件元数据查询          |

### Vulkan 后端

| 报告                                                             | 描述                                                             |
|----------------------------------------------------------------|----------------------------------------------------------------|
| [vulkan_Device.md](vulkan_Device.md)                           | `DeviceManager` Vulkan 实例/设备/队列管理，内存类型查询                       |
| [vulkan_SwapChain.md](vulkan_SwapChain.md)                     | `SwapChain` 交换链管理，图像获取/呈现，格式选择                                 |
| [vulkan_CommandBuffer.md](vulkan_CommandBuffer.md)             | `CommandBuffer` 命令缓冲封装，图像 layout 转换工具                          |
| [vulkan_Synchronization.md](vulkan_Synchronization.md)         | `Semaphore/Fence/TimelineSemaphore/FrameSyncManager` 同步原语      |
| [vulkan_Pipeline.md](vulkan_Pipeline.md)                       | `Pipeline` 图形管线创建，着色器阶段，固定功能状态                                 |
| [vulkan_RenderPassManager.md](vulkan_RenderPassManager.md)     | `RenderPassManager` RenderPass 哈希缓存，Present/Offscreen Pass     |
| [vulkan_Buffer.md](vulkan_Buffer.md)                           | `Buffer` Vulkan 缓冲 RAII 封装，映射/复制操作                             |
| [vulkan_Image.md](vulkan_Image.md)                             | `Image` Vulkan 图像 RAII 封装，layout 转换，数据上传                       |
| [vulkan_DepthBuffer.md](vulkan_DepthBuffer.md)                 | `DepthBuffer` 深度缓冲创建，格式自动选择                                    |
| [vulkan_Framebuffer.md](vulkan_Framebuffer.md)                 | `Framebuffer/FramebufferPool` Framebuffer 管理，交换链 Framebuffer 池 |
| [vulkan_UniformBuffer.md](vulkan_UniformBuffer.md)             | `UniformBuffer<T>` 模板 Uniform 缓冲，多帧缓冲管理                        |
| [vulkan_ShaderModule.md](vulkan_ShaderModule.md)               | `ShaderModule` SPIR-V 着色器模块加载，Magic Number 验证                  |
| [vulkan_VulkanError.md](vulkan_VulkanError.md)                 | `VulkanError` 异常类，`VK_CHECK` 宏，VkResult 字符串映射                  |
| [vulkan_CoordinateTransform.md](vulkan_CoordinateTransform.md) | `CoordinateTransform` OpenGL→Vulkan 投影矩阵 Y 轴翻转工具               |

### 渲染层

| 报告                                                                   | 描述                                                                    |
|----------------------------------------------------------------------|-----------------------------------------------------------------------|
| [rendering_RenderGraph.md](rendering_RenderGraph.md)                 | `RenderGraph` 声明式渲染图，节点编译与执行                                          |
| [rendering_RenderGraphPass.md](rendering_RenderGraphPass.md)         | `RenderPassBase/GeometryRenderPass/PresentRenderPass` 渲染 Pass 基类与内置实现 |
| [rendering_RenderGraphResource.md](rendering_RenderGraphResource.md) | `RenderGraphResource` 渲染图资源描述与 barrier 管理                             |
| [rendering_RenderGraphTypes.md](rendering_RenderGraphTypes.md)       | `ImageHandle/BufferHandle/Format` 渲染图类型系统                             |
| [rendering_CubeRenderPass.md](rendering_CubeRenderPass.md)           | `CubeRenderPass` 立方体网格渲染 Pass，MVP 矩阵更新                                |
| [rendering_ViewportRenderPass.md](rendering_ViewportRenderPass.md)   | `ViewportRenderPass` Viewport 离屏渲染 Pass，Framebuffer resize            |
| [rendering_Material.md](rendering_Material.md)                       | `Material` 材质系统，多 RenderPass 管线缓存，参数 Uniform 更新                       |
| [rendering_MaterialLoader.md](rendering_MaterialLoader.md)           | `MaterialLoader` JSON 材质文件解析，着色器/纹理路径解析                               |
| [rendering_Viewport.md](rendering_Viewport.md)                       | `Viewport` 渲染 Viewport 抽象，ImGui 纹理 ID 管理，宽高比查询                        |
| [rendering_RenderTarget.md](rendering_RenderTarget.md)               | `RenderTarget` 离屏渲染目标，颜色/深度附件管理，resize 支持                             |
| [rendering_Mesh.md](rendering_Mesh.md)                               | `Mesh` GPU 网格数据，顶点/索引缓冲上传                                             |
| [rendering_ObjLoader.md](rendering_ObjLoader.md)                     | `ObjLoader` 手写 OBJ 解析器，UV/法线/三角化支持                                    |
| [rendering_TextureLoader.md](rendering_TextureLoader.md)             | `TextureLoader` stb_image 纹理加载，Mipmap 生成，Staging 上传                   |
| [rendering_ResourceManager.md](rendering_ResourceManager.md)         | `ResourceManager` 统一资源管理门面，四类资源加载/卸载/缓存                               |
| [rendering_Scene.md](rendering_Scene.md)                             | `Scene` 场景实体容器，空间索引接口，视锥裁剪/射线检测                                       |
| [rendering_SceneViewport.md](rendering_SceneViewport.md)             | `SceneViewport` 离屏渲染到纹理，ImGui Viewport 显示                             |
| [rendering_ShaderManager.md](rendering_ShaderManager.md)             | `ShaderManager` 着色器程序加载、编译、缓存与热重载                                     |
| [rendering_CameraController.md](rendering_CameraController.md)       | `CameraController/OrbitCameraController` 相机控制器，输入解耦                   |

---

## 全局高优先级问题汇总

以下 🔴 高风险问题影响面广，建议优先修复：

### 立即修复（影响运行稳定性）

| # | 问题                                                       | 涉及文件                  |
|---|----------------------------------------------------------|-----------------------|
| 1 | `Material::set_vec2/set_int/set_bool` 声明但未实现，导致**链接错误**  | `Material.cpp`        |
| 2 | `compile_glsl/compile_hlsl` 声明但未实现，导致**链接错误**            | `ShaderManager.cpp`   |
| 3 | `TimelineSemaphore` 实际创建的是普通 Binary Semaphore，API 语义完全错误 | `Synchronization.cpp` |
| 4 | `Image::transition_layout()` 不实际执行 pipeline barrier      | `Image.cpp`           |
| 5 | `Image::upload_data()` 为空实现，纹理数据无法上传到 GPU                | `Image.cpp`           |
| 6 | `Scene::destroy_entity()` 后外部悬空指针                        | `Scene.cpp`           |

### 高优先级修复（影响性能或正确性）

| #  | 问题                                                                      | 涉及文件                                                  |
|----|-------------------------------------------------------------------------|-------------------------------------------------------|
| 7  | 每帧 `vkQueueWaitIdle` 阻塞图形队列，破坏 CPU/GPU 并行流水线                            | `main.cpp`                                            |
| 8  | `ResourceManager` 所有加载方法均为占位符，资源无 GPU 数据                                | `ResourceManager.cpp`                                 |
| 9  | `ShaderManager::compile_shader()` 输出空 bytecode 但标记为成功                   | `ShaderManager.cpp`                                   |
| 10 | `ViewportRenderPass::cleanup_framebuffer()` 同时销毁 render_pass，resize 后崩溃 | `ViewportRenderPass.cpp`                              |
| 11 | `SceneViewport` resize 后旧 ImGui 描述符集未释放，描述符池泄漏                          | `SceneViewport.cpp`                                   |
| 12 | 多处硬编码绝对路径 `D:/TechArt/Vulkan/`，跨机器无法运行                                  | `main.cpp`, `TextureLoader.cpp`, `MaterialLoader.cpp` |

### 代码质量问题（影响可维护性）

| #  | 问题                                                       | 涉及文件                             |
|----|----------------------------------------------------------|----------------------------------|
| 13 | 每帧输出大量 `logger::info`，性能损失且日志无可读性                        | `main.cpp`, `CubeRenderPass.cpp` |
| 14 | `Logger` 无日志级别过滤机制                                       | `Logger.hpp`                     |
| 15 | `on_window_resize()`、`SceneViewport::resize()` 等函数格式严重损坏 | `main.cpp`, `SceneViewport.cpp`  |
| 16 | 顶点缓冲普遍使用 `HOST_VISIBLE` 内存，应改为 DEVICE_LOCAL + staging    | `Mesh.cpp`, `main.cpp`           |

---

## 统计摘要

| 模块        |  报告数   | 🔴 高风险总计 | 🟡 中风险总计 | 🟢 低风险总计 |
|-----------|:------:|:--------:|:--------:|:--------:|
| 应用入口      |   1    |    4     |    4     |    2     |
| 应用层       |   2    |    1     |    4     |    4     |
| 编辑器       |   2    |    2     |    4     |    4     |
| 核心层       |   3    |    2     |    4     |    5     |
| 平台层       |   3    |    3     |    6     |    6     |
| Vulkan 后端 |   14   |    20    |    22    |    24    |
| 渲染层       |   18   |    30    |    34    |    28    |
| **合计**    | **43** |  **62**  |  **78**  |  **73**  |

---

*报告由 WorkBuddy 自动分析生成，基于静态代码审查。建议结合运行时调试（RenderDoc / Validation Layers）进一步验证。*

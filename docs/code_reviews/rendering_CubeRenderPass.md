# 代码审查报告：CubeRenderPass

**文件路径**

- `include/rendering/render_graph/CubeRenderPass.hpp`
- `src/rendering/render_graph/CubeRenderPass.cpp`

---

## 功能概述

`CubeRenderPass` 是当前项目实际使用的具体渲染 Pass：

- **Material 绑定**：通过 `weak_ptr<Material>` 引用材质，每帧 `lock()` 获取强引用
- **MVP 矩阵**：使用 push constant 传递 model/view/projection 矩阵
- **相机集成**：通过 `weak_ptr<OrbitCamera>` 获取视图/投影矩阵
- **Mesh 绘制**：绑定顶点/索引缓冲，执行 `vkCmdDrawIndexed`

---

## 关键设计

| 特性            | 说明                                                   |
|---------------|------------------------------------------------------|
| weak_ptr 引用   | 材质和相机均用 weak_ptr，避免循环引用和悬空访问                         |
| Push Constant | MVP 矩阵通过 push constant 上传，无需 uniform buffer per-draw |
| 动态 Viewport   | execute 时动态设置 viewport/scissor                       |

---

## 潜在问题

### 🔴 高风险

1. **每帧大量 `logger::info` 输出（性能问题）**  
   `execute()` 中有多条 `logger::info` 输出（如 "CubeRenderPass executing"、MVP 矩阵值等），这些在每帧 60fps 下会产生每秒
   60+ 次日志输出，严重影响性能（每次日志都会 flush stdout）。  
   **建议**：删除或改为 debug 级别，并使用编译期关闭的 debug log 宏。

2. **`RenderPassBase::execute(CommandBuffer&)` 被调用时 `dynamic_cast` 风险**  
   若通过基类接口调用 `execute(CommandBuffer&)`，基类打印警告后返回，实际渲染不执行，但不会报错。

### 🟡 中风险

3. **`material_ref_.lock()` 失败时仅打 error 不做恢复**  
   材质 weak_ptr 失效时只记录错误并返回，该帧场景内容为空，对用户不友好。  
   **建议**：在材质加载完成前显示默认材质（wireframe 或纯色）。

4. **Push constant 布局硬编码**  
   Push constant 范围和偏移量直接写在代码中，若材质的管线布局中 push constant 大小不匹配，Vulkan 会报 validation
   error，但代码中无明确的布局验证。

### 🟢 低风险

5. **`setup()` 方法未声明资源依赖**  
   `setup(RenderGraphBuilder&)` 中没有声明任何读写资源，Render Graph 无法为此 Pass 自动生成正确的 barrier，同步完全依赖外部手动管理。

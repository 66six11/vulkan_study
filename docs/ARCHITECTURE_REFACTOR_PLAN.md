# Vulkan Engine 架构重构计划

**状态**: 🔄 进行中  
**开始日期**: 2026-03-20  
**目标**: 分离 Editor 和 Runtime，清理命名空间，消除跨层依赖

---

## 1. 重构目标

### 1.1 当前问题

```
当前架构问题：
┌─────────────────────────────────────────────────────────┐
│  Editor (include/editor/)                               │
│  ⚠️ 直接包含 vulkan/device/Device.hpp                   │
│  ⚠️ 暴露 VkRenderPass, VkCommandBuffer 到 public API    │
├─────────────────────────────────────────────────────────┤
│  Rendering (include/rendering/)                         │
│  ⚠️ SceneRenderer 和 UIRenderer 职责混杂                │
│  ⚠️ RenderGraph 暴露 Vulkan 类型                        │
├─────────────────────────────────────────────────────────┤
│  Vulkan (include/vulkan/)                               │
│  ✅ 类型安全包装良好                                     │
│  ⚠️ 但命名空间过于具体，不利于未来扩展                   │
├─────────────────────────────────────────────────────────┤
│  Core/Platform (include/core, include/platform)         │
│  ⚠️ 分散在 include/ 和 src/，不如 engine/ 结构清晰      │
└─────────────────────────────────────────────────────────┘
```

### 1.2 目标架构

```
目标架构：
┌─────────────────────────────────────────────────────────┐
│  Apps (apps/editor/)                                    │
│  ✅ 干净的入口点，只使用 engine::editor::EditorApp       │
├─────────────────────────────────────────────────────────┤
│  Editor Layer (engine/editor/)                          │
│  ✅ 只依赖 rendering::Renderer                          │
│  ✅ ❌ 禁止直接使用任何 RHI/Vulkan 类型                  │
├─────────────────────────────────────────────────────────┤
│  Rendering Layer (engine/rendering/)                    │
│  ✅ 抽象的 CommandBuffer、ResourceHandle                │
│  ✅ 使用 rhi::CommandBuffer（包装类型）                  │
├─────────────────────────────────────────────────────────┤
│  RHI Layer (engine/rhi/)                                │
│  ✅ 所有 VkXXX 类型在此层封装                            │
│  ✅ 当前仅 Vulkan 实现，接口已准备好多 API 支持          │
├─────────────────────────────────────────────────────────┤
│  Platform Layer (engine/platform/)                      │
│  ✅ 窗口、输入、文件系统抽象                             │
├─────────────────────────────────────────────────────────┤
│  Core Layer (engine/core/)                              │
│  ✅ 数学、内存、日志、工具函数                           │
└─────────────────────────────────────────────────────────┘
```

---

## 2. 命名空间变更

| 旧命名空间 | 新命名空间 | 说明 |
|-----------|-----------|------|
| `vulkan_engine::core` | `engine::core` | 简化前缀 |
| `vulkan_engine::platform` | `engine::platform` | 简化前缀 |
| `vulkan_engine::vulkan` | `engine::rhi::vulkan` | 重命名为 RHI |
| `vulkan_engine::rendering` | `engine::rendering` | 简化前缀 |
| `vulkan_engine::editor` | `engine::editor` | 简化前缀 |

---

## 3. 目录结构重构

### 3.1 新目录布局

```
D:\TechArt\Vulkan\
├───apps\
│   └───editor\n│       ├───main.cpp
│       └───bootstrap\n├───engine
e                        # 运行时引擎代码
│   ├───core\
│   │   ├───include\engine\core\       # 数学、内存、工具
│   │   └───src\
│   ├───platform\
│   │   ├───include\engine\platform\   # 窗口、输入、文件系统
│   │   └───src\
│   ├───rhi\
│   │   ├───include\engine\rhi\        # RHI 接口
│   │   │   └───vulkan\                # Vulkan 实现
│   │   └───src\vulkan\
│   ├───rendering\
│   │   ├───include\engine\rendering\  # 渲染层
│   │   └───src\
│   └───editor\
│       ├───include\engine\editor\     # 编辑器运行时
│       └───src\
├───CMake\                             # CMake 配置
├───docs\                              # 文档
├───shaders\                           # 着色器
└───tests\                             # 测试
```

---

## 4. 执行计划

### Phase 1: 创建目录结构 ✅
**状态**: 已完成 (2026-03-20 17:06)  
**任务**: 创建所有新的 engine/ 子目录
- ✅ engine/core/
- ✅ engine/platform/
- ✅ engine/rhi/
- ✅ engine/rendering/
- ✅ engine/editor/

### Phase 2: 迁移 Core 模块 ✅
**状态**: 已完成 (2026-03-20 17:10)  
**任务**:
- [x] 移动 include/core → engine/core/include/engine/core
  - ✅ Logger.hpp (utils/) - 命名空间: engine::logger
  - ✅ Vector.hpp (math/) - 命名空间: engine::math
  - ✅ Camera.hpp (math/) - 命名空间: engine::core
- [x] 更新命名空间 vulkan_engine::* → engine::*
- [ ] 更新 CMakeLists.txt (Phase 7 统一处理)

### Phase 3: 迁移 Platform 模块 ⏳
**状态**: 待开始  
**任务**:
- [ ] 移动 include/platform → engine/platform/include/engine/platform
- [ ] 移动 src/platform → engine/platform/src
- [ ] 更新命名空间
- [ ] 更新 CMakeLists.txt

### Phase 4: 重构 Vulkan → RHI ⏳
**状态**: 待开始  
**任务**:
- [ ] 创建 engine/rhi 目录结构
- [ ] 移动 include/vulkan → engine/rhi/include/engine/rhi/vulkan
- [ ] 移动 src/vulkan → engine/rhi/src/vulkan
- [ ] 创建 engine/rhi/include/engine/rhi/ 接口头文件
- [ ] 更新命名空间 vulkan_engine::vulkan → engine::rhi::vulkan
- [ ] 更新 CMakeLists.txt

### Phase 5: 重构 Rendering 模块 ⏳
**状态**: 待开始  
**任务**:
- [ ] 移动文件到 engine/rendering
- [ ] 消除 Vulkan 类型暴露
- [ ] 分离 SceneRenderer 和 UIRenderer
- [ ] 更新命名空间
- [ ] 更新 CMakeLists.txt

### Phase 6: 重构 Editor 模块 ⏳
**状态**: 待开始  
**任务**:
- [ ] 移动文件到 engine/editor
- [ ] 消除直接 Vulkan 依赖
- [ ] Editor::render_to_command_buffer() 使用包装类型
- [ ] 更新命名空间
- [ ] 更新 CMakeLists.txt

### Phase 7: 更新 CMake 配置 ⏳
**状态**: 待开始  
**任务**:
- [ ] 创建新的 CMake/Modules/ 配置
- [ ] 更新根 CMakeLists.txt
- [ ] 修复所有 include 路径

### Phase 8: 清理和验证 ⏳
**状态**: 待开始  
**任务**:
- [ ] 删除旧的 include/ 和 src/ 目录（确认迁移完成后）
- [ ] 运行构建验证
- [ ] 修复编译错误

---

## 5. 关键代码变更

### 5.1 Editor 消除 Vulkan 依赖

**当前代码** (include/editor/Editor.hpp):
```cpp
#include "vulkan/device/Device.hpp"        // ❌ 直接使用 Vulkan
#include "vulkan/device/SwapChain.hpp"     // ❌ 直接使用 Vulkan

void render_to_command_buffer(VkCommandBuffer command_buffer);  // ❌ Vulkan 裸类型
void recreate_render_pass(VkRenderPass render_pass, uint32_t image_count);  // ❌ Vulkan 裸类型
```

**目标代码** (engine/editor/include/engine/editor/Editor.hpp):
```cpp
#include "engine/rendering/Renderer.hpp"   // ✅ 只依赖 Rendering 层

namespace engine::editor {

class Editor {
public:
    void initialize(std::shared_ptr<rendering::Renderer> renderer,
                   std::shared_ptr<platform::Window> window);
    
    void render_ui(rendering::CommandBuffer& cmd);  // ✅ 使用包装类型
};

}
```

### 5.2 Rendering 提供抽象接口

**新增** (engine/rendering/include/engine/rendering/CommandBuffer.hpp):
```cpp
namespace engine::rendering {

// 对 RHI CommandBuffer 的包装
class CommandBuffer {
public:
    void begin_render_pass(const RenderPassDesc& desc);
    void bind_pipeline(const Pipeline& pipeline);
    void draw(uint32_t vertex_count, uint32_t instance_count = 1);
    void end_render_pass();
    
    // 仅供内部使用
    rhi::CommandBuffer& native() { return *native_cmd_; }
    
private:
    std::shared_ptr<rhi::CommandBuffer> native_cmd_;
};

}
```

### 5.3 RHI 接口抽象

**新增** (engine/rhi/include/engine/rhi/CommandBuffer.hpp):
```cpp
namespace engine::rhi {

// RHI 层命令缓冲 - 当前 Vulkan 实现
class CommandBuffer {
public:
    void begin();
    void end();
    void begin_render_pass(const RenderPassDesc& desc);
    // ...
    
    // 内部获取原生句柄
    VkCommandBuffer native_handle() const { return cmd_; }
    
private:
    VkCommandBuffer cmd_;
};

}
```

---

## 6. 依赖关系

### 6.1 新的模块依赖图

```
                    ┌─────────────┐
                    │   Editor    │
                    │   (App)     │
                    └──────┬──────┘
                           │ uses
                           ▼
                    ┌─────────────┐
                    │   editor    │
                    │   (lib)     │
                    └──────┬──────┘
                           │ uses
                           ▼
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│  rendering  │◄────│  rendering  │────►│   rhi       │
│   (scene)   │     │  (composed) │     │  (vulkan)   │
└─────────────┘     └──────┬──────┘     └──────┬──────┘
                           │                    │
                           ▼                    ▼
                    ┌─────────────┐     ┌─────────────┐
                    │  platform   │     │  platform   │
                    └──────┬──────┘     └──────┬──────┘
                           │                    │
                           ▼                    ▼
                    ┌─────────────┐     ┌─────────────┐
                    │    core     │◄────│    core     │
                    └─────────────┘     └─────────────┘
```

---

## 7. 构建验证清单

- [ ] Core 模块编译通过
- [ ] Platform 模块编译通过
- [ ] RHI 模块编译通过
- [ ] Rendering 模块编译通过
- [ ] Editor 模块编译通过
- [ ] Editor App 编译通过
- [ ] 运行测试无崩溃

---

## 8. 风险与注意事项

### 8.1 主要风险

1. **文件移动导致 Git 历史丢失** → 使用 `git mv` 保留历史
2. **头文件包含路径变更** → 批量替换 #include 路径
3. **命名空间冲突** → 使用 IDE 重构功能或 sed 批量替换
4. **CMake 配置错误** → 分阶段验证每个模块

### 8.2 回滚策略

如果在重构过程中遇到问题：
1. 保留原始 include/ 和 src/ 目录直到验证完成
2. 使用 Git 分支进行重构
3. 每个 Phase 完成后提交一次

---

## 更新日志

| 日期 | 操作 | 状态 |
|-----|------|-----|
| 2026-03-20 | 创建重构计划文档 | ✅ 完成 |

人为补充：需要模块化，预留后续的引擎中的音频模块，脚本模块，物理系统，还有打包流程等等


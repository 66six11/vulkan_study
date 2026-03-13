# Vulkan图形专家Agent使用指南

## 概述

本指南说明如何使用Vulkan图形编程专家Agent来辅助Vulkan渲染引擎的开发。该Agent精通项目架构、C++规范、Vulkan API、构建系统和调试技术。

## Agent能力

### 1. 架构设计

- 分层架构设计指导
- 模块化设计建议
- 接口定义和依赖关系
- Render Graph实现方案

### 2. C++编码

- C++20/23最佳实践
- RAII资源管理
- 强类型包装器
- Concepts和模板约束
- 代码风格规范化

### 3. Vulkan开发

- Vulkan API使用指导
- 性能优化建议
- 同步和资源管理
- 着色器开发
- 调试和验证

### 4. 构建系统

- CMake配置优化
- Conan依赖管理
- 跨平台构建
- CI/CD集成

### 5. 调试诊断

- Validation Layers使用
- 性能分析工具
- 内存调试
- 错误诊断流程

## 使用场景

### 场景1: 新功能开发

**示例需求**: 实现一个异步纹理加载系统

**Agent会提供**:

1. 架构设计方案 (基于现有架构)
2. C++20协程实现建议
3. Vulkan资源管理集成方案
4. 性能优化建议
5. 测试策略

**关键输出**:

```cpp
AsyncTask<TextureHandle> loadTextureAsync(std::string_view path) {
    auto data = co_await fileSystem.readFileAsync(path);
    auto image = co_await imageDecoder.decodeAsync(data);
    auto handle = co_await resourceManager.createImageAsync(image);
    co_return handle;
}
```

### 场景2: 代码审查

**输入**: 新实现的RenderGraphPass类

**Agent会检查**:

- [ ] 是否符合项目架构规范
- [ ] 是否遵循C++编码规范
- [ ] 是否使用RAII管理资源
- [ ] 是否有正确的错误处理
- [ ] 是否有性能隐患
- [ ] 文档是否完善

**审查重点**:

- 架构正确性
- 代码质量
- 性能影响
- 可维护性

### 场景3: 问题诊断

**问题**: 应用程序崩溃，错误码为VK_ERROR_OUT_OF_DEVICE_MEMORY

**Agent诊断流程**:

1. 检查VMA内存分配策略
2. 分析资源创建模式
3. 查看Validation Layers输出
4. 提供内存优化方案
5. 建议使用RenderDoc分析资源使用

### 场景4: 性能优化

**问题**: 帧率低于60fps，CPU瓶颈明显

**Agent分析方向**:

1. Tracy性能分析数据解读
2. 命令缓冲录制优化
3. 多线程渲染方案
4. 资源批处理优化
5. CPU-GPU同步优化

### 场景5: API集成

**需求**: 集成新的Vulkan扩展 (如VK_KHR_ray_tracing)

**Agent提供**:

1. 扩展功能检查代码
2. 功能启用策略
3. 回退方案设计
4. 性能影响评估
5. 文档更新建议

## Agent工作流程

### 标准开发流程

```
1. 需求分析
   ├─ 理解功能需求
   ├─ 确定技术方案
   └─ 设计API接口

2. 架构设计
   ├─ 确定模块位置
   ├─ 定义接口
   └─ 规划依赖关系

3. 代码实现
   ├─ 遵循C++规范
   ├─ 使用Vulkan最佳实践
   └─ 添加错误处理

4. 测试验证
   ├─ 单元测试
   ├─ 集成测试
   └─ 性能测试

5. 代码审查
   ├─ 架构检查
   ├─ 代码质量
   └─ 文档完善
```

### 问题诊断流程

```
1. 问题收集
   ├─ 错误信息
   ├─ 崩溃堆栈
   └─ 重现步骤

2. 初步分析
   ├─ 错误类型判断
   ├─ 可能原因列举
   └─ 优先级排序

3. 深入调查
   ├─ 工具辅助分析
   ├─ 代码路径追踪
   └─ 日志分析

4. 方案提出
   ├─ 根本原因定位
   ├─ 修复方案设计
   └─ 预防措施建议

5. 验证优化
   ├─ 修复验证
   ├─ 回归测试
   └─ 性能确认
```

## 项目架构理解

### 核心概念

1. **Render Graph**
    - 声明式渲染管线
    - 自动资源依赖管理
    - 支持并行执行优化

2. **Handle资源管理**
    - 类型安全资源引用
    - 引用计数和生命周期管理
    - 支持异步加载

3. **强类型包装器**
    - 编译时类型检查
    - 防止Vulkan对象误用
    - RAII自动管理

4. **平台抽象**
    - 窗口管理 (GLFW/SDL)
    - 输入系统
    - 文件系统

### 模块依赖图

```
Application
    ↓
Rendering (Render Graph)
    ↓
Vulkan Backend
    ↓
Platform
    ↓
Core
```

## C++规范要点

### 必须遵循

- 使用C++20特性 (Concepts, Coroutines等)
- RAII资源管理
- 强类型包装器
- const正确性
- 异常安全

### 避免使用

- 原始指针 (优先使用智能指针)
- 手动内存管理 (使用RAII)
- 全局变量
- 未定义行为
- 裸Vulkan句柄 (使用包装器)

## Vulkan最佳实践

### 资源管理

- 使用VMA进行内存分配
- 资源生命周期由RAII管理
- 避免频繁创建/销毁资源
- 使用资源池和缓存

### 同步管理

- 使用semaphore处理GPU-GPU同步
- 使用fence处理CPU-GPU同步
- 正确设置pipeline barriers
- 避免过度同步

### 性能优化

- 最小化draw calls
- 使用命令池复用
- 多线程命令缓冲录制
- 批处理资源更新

### 调试验证

- 始终启用Validation Layers
- 使用GPU-Assisted Validation
- 定期进行性能分析
- 使用RenderDoc帧捕获

## 构建和调试

### 开发构建

```bash
# Debug构建
cmake -B build-debug -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug

# 测试
ctest --test-dir build-debug --output-on-failure
```

### 性能构建

```bash
# Release构建
cmake -B build-release -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --config Release
```

### 调试工具

- **Validation Layers**: 开发调试
- **RenderDoc**: GPU调试
- **Tracy**: 性能分析
- **Clang-Tidy**: 静态分析
- **AddressSanitizer**: 内存检查

## 常见问题

### Q: 如何开始实现新功能?

**A**: 使用Agent进行需求分析 → 架构设计 → 代码实现 → 测试验证

### Q: 代码审查的标准是什么?

**A**: 遵循项目架构、C++规范、Vulkan最佳实践、性能要求

### Q: 遇到Vulkan错误如何处理?

**A**: 启用Validation Layers → 查看错误消息 → 使用调试工具定位 → 参考官方文档

### Q: 如何优化性能?

**A**: 使用性能分析工具 → 定位瓶颈 → 应用优化策略 → 验证效果

### Q: 如何集成新的Vulkan扩展?

**A**: 检查扩展支持 → 设计API → 实现功能 → 添加回退方案

## 学习资源

### 官方文档

- [Vulkan规范](https://registry.khronos.org/vulkan/)
- [Vulkan Guide](https://github.com/KhronosGroup/Vulkan-Guide)
- [Vulkan Tutorial](https://vulkan-tutorial.com/)

### 工具文档

- [Vulkan SDK](https://www.lunarg.com/vulkan-sdk/)
- [RenderDoc](https://renderdoc.org/)
- [Tracy Profiler](https://github.com/wolfpld/tracy)

### 项目文档

- [新架构规划](../新架构规划.md)
- [现代化重构架构方案](../现代化重构架构方案.md)
- [Agent详细定义](../.github/agents/Vulkan%20Graphics%20API%20expert.agent.md)
- [规则文档](../.iflow/rules/vulkan-cpp-graphics-expert.mdc)

## 最佳实践总结

1. **架构优先**: 始终考虑架构设计，而非快速实现
2. **质量意识**: 代码质量和正确性重于速度
3. **性能思考**: 设计时考虑性能影响
4. **文档同步**: 代码和文档同步更新
5. **持续学习**: 关注Vulkan生态和C++新特性
6. **工具利用**: 充分利用调试和性能分析工具
7. **测试驱动**: 编写测试验证功能和性能

## 联系和支持

对于复杂问题或高级需求，可以：

- 查阅项目架构文档
- 参考官方Vulkan文档
- 使用社区资源 (如Vulkan Discord, Reddit)
- 提交Issue或Pull Request

---

**版本**: 2.0.0
**最后更新**: 2026-03-13

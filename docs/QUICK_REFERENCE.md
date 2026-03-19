# Vulkan图形专家Agent快速参考

## 📋 项目架构速览

```
Application Layer     → 应用逻辑
  ↓
Render Abstraction    → Render Graph, 资源管理
  ↓
Vulkan Backend        → Vulkan实现
  ↓
Platform Layer        → 窗口, 输入, 文件系统
  ↓
Core System           → 数学, 内存, 工具
```

## 🔑 核心概念

### Render Graph

```cpp
class RenderGraph {
    struct Pass {
        std::string name;
        std::vector<ResourceHandle> inputs, outputs;
        std::function<void(CommandBuffer&)> execute;
    };
    void addPass(Pass pass);
    void compile();
    void execute();
};
```

### Handle资源管理

```cpp
template<typename T>
class Handle {
    uint32_t id_, generation_;
    bool isValid() const;
};

ResourceManager::createBuffer(const BufferDesc&);
```

### 强类型包装器

```cpp
template<typename Tag>
class VulkanObject {
    VkHandle handle_;
    operator VkHandle() const;
};

using Instance = VulkanObject<struct InstanceTag>;
```

## 📝 C++规范

### 命名约定

- 命名空间: `vulkan_engine`
- 类名: `PascalCase`
- 函数名: `camelCase`
- 成员: `camelCase_`
- 常量: `UPPER_SNAKE_CASE`

### 核心特性

- **C++20**: Concepts, Coroutines, Modules
- **RAII**: 所有资源自动管理
- **移动语义**: 优先使用移动
- **const正确性**: const, constexpr

### 代码模板

```cpp
// RAII资源管理
class VulkanDevice {
    VkDevice handle_;
    ~VulkanDevice() { vkDestroyDevice(handle_, nullptr); }
    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice(VulkanDevice&&) noexcept = default;
};

// Concept约束
template<typename T>
concept Renderable = requires(T t) {
    { t.getVertexBuffer() } -> std::convertible_to<BufferHandle>;
};

// Coroutines异步
AsyncTask<MeshHandle> loadMeshAsync(std::string_view path) {
    auto data = co_await fileSystem.readFileAsync(path);
    co_return handle;
}
```

## 🎯 Vulkan最佳实践

### 资源管理

- 使用VMA (Vulkan Memory Allocator)
- RAII + 智能指针
- 资源池和缓存
- 避免频繁创建/销毁

### 同步管理

- Semaphore: GPU-GPU同步
- Fence: CPU-GPU同步
- Pipeline Barriers: 执行顺序
- 避免过度同步

### 性能优化

- 最小化draw calls
- 多线程录制
- 批处理资源更新
- 命令池复用

### 调试验证

- VK_LAYER_KHRONOS_validation
- GPU-Assisted Validation
- 同步验证层
- 详细调试消息

## 🔧 构建命令

### CMake构建

```bash
# 配置
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# 构建
cmake --build build --config Release

# 测试
ctest --test-dir build --output-on-failure

# 安装
cmake --install build --prefix install
```

### Conan构建

```bash
# 安装依赖
conan install . --build=missing -s build_type=Release

# 创建包
conan create . --build=missing

# 清理
conan remove "*" -c -f
```

### 依赖版本

- Vulkan: 1.3.268+
- GLFW: 3.3.8+
- GLM: 0.9.9.8+
- stb: 最新
- Taskflow: 3.6.0+

## 🐛 调试工具

### Validation Layers

```cpp
const char* layers[] = {
    "VK_LAYER_KHRONOS_validation"
};
VkInstanceCreateInfo info{
    .enabledLayerCount = 1,
    .ppEnabledLayerNames = layers
};
```

### 工具集

- **RenderDoc**: GPU调试, 帧捕获
- **Tracy**: CPU/GPU性能分析
- **NSight**: NVIDIA性能工具
- **Clang-Tidy**: 静态分析
- **AddressSanitizer**: 内存检查

### 日志系统

```cpp
enum class LogLevel { Trace, Debug, Info, Warning, Error, Fatal };
void log(LogLevel level, std::string_view message);
```

## 📚 API文档资源

### 官方资源

- [Vulkan Registry](https://registry.khronos.org/vulkan/)
- [Vulkan Guide](https://github.com/KhronosGroup/Vulkan-Guide)
- [Vulkan Tutorial](https://vulkan-tutorial.com/)
- [LunarG SDK](https://www.lunarg.com/vulkan-sdk/)

### 关键API

- **实例/设备**: VkInstance, VkPhysicalDevice, VkDevice
- **交换链**: VkSwapchainKHR, VkSurfaceKHR
- **命令缓冲**: VkCommandBuffer, VkCommandPool
- **同步**: VkFence, VkSemaphore, VkEvent
- **描述符**: VkDescriptorSet, VkDescriptorSetLayout
- **管线**: VkPipeline, VkPipelineLayout, VkRenderPass

## ✅ 代码审查清单

### 架构

- [ ] 符合分层架构
- [ ] 模块职责单一
- [ ] 接口定义合理
- [ ] 依赖关系正确

### C++

- [ ] 遵循命名规范
- [ ] RAII资源管理
- [ ] 现代C++特性
- [ ] 异常安全
- [ ] const正确性

### Vulkan

- [ ] 错误检查完整
- [ ] 资源生命周期
- [ ] 同步原语正确
- [ ] 性能优化

### 构建

- [ ] CMake配置
- [ ] 依赖清晰
- [ ] 跨平台兼容
- [ ] 文档配置

## 🚀 性能优化检查

### GPU

- [ ] 减少draw calls
- [ ] 着色器优化
- [ ] 管线状态优化
- [ ] 带宽优化

### CPU

- [ ] 多线程渲染
- [ ] 异步资源加载
- [ ] 减少同步点
- [ ] 缓存优化

### 内存

- [ ] VMA智能分配
- [ ] 资源复用
- [ ] 内存池管理
- [ ] 避免碎片化

## 🛠️ 常用命令

### 格式化代码

```bash
find . -name "*.cpp" -o -name "*.hpp" | xargs clang-format -i
```

### 静态分析

```bash
clang-tidy src/**/*.cpp -- -std=c++20 -I include/
```

### 清理构建

```bash
rm -rf build build-debug build-release
```

### 运行测试

```bash
ctest --test-dir build-debug --verbose
```

## 📖 项目文件结构

```
include/
├── application/     # 应用层接口
├── core/           # 核心系统接口
├── platform/       # 平台抽象接口
├── rendering/      # 渲染系统接口
└── vulkan/         # Vulkan后端接口

src/
├── application/     # 应用层实现
├── core/           # 核心系统实现
├── platform/       # 平台抽象实现
├── rendering/      # 渲染系统实现
├── vulkan/         # Vulkan后端实现
└── main.cpp        # 入口文件

docs/
├── README.md       # 项目文档
├── AGENT_GUIDE.md  # Agent使用指南
└── QUICK_REFERENCE.md # 快速参考 (本文件)

.github/agents/
└── Vulkan Graphics API expert.agent.md  # Agent定义

.workbuddy/rules/
└── vulkan-cpp-graphics-expert.mdc  # 规则文档
```

## 💡 最佳实践

1. **架构优先**: 设计时考虑架构, 而非快速实现
2. **质量意识**: 代码质量重于开发速度
3. **性能思考**: 设计时考虑性能影响
4. **文档同步**: 代码和文档同步更新
5. **工具利用**: 充分使用调试和分析工具
6. **测试驱动**: 编写测试验证功能
7. **持续学习**: 关注Vulkan和C++新特性

## 🚨 常见错误

### Vulkan错误

- **VK_ERROR_OUT_OF_DEVICE_MEMORY**: 检查VMA配置, 优化内存使用
- **VK_ERROR_DEVICE_LOST**: 驱动问题, 更新驱动, 检查资源使用
- **VK_ERROR_OUT_OF_DATE_KHR**: 交换链重建, 窗口尺寸变化
- **Validation Error**: 启用详细验证, 检查API使用

### C++错误

- **编译错误**: 检查C++版本, 编译器支持
- **链接错误**: 确认依赖链接, 检查CMake配置
- **运行时错误**: 使用调试器, 检查日志
- **内存泄漏**: AddressSanitizer, Valgrind

### 构建错误

- **依赖缺失**: Conan安装, vcpkg配置
- **配置错误**: CMake选项, 环境变量
- **平台差异**: 跨平台适配, 条件编译

## 🎓 学习路径

1. **基础阶段**
    - 学习Vulkan基础概念
    - 理解项目架构设计
    - 掌握C++20特性

2. **进阶阶段**
    - 深入Render Graph
    - 性能优化技术
    - 多线程渲染

3. **高级阶段**
    - 高级Vulkan特性
    - 架构设计模式
    - 跨平台适配

---

**记住**: 这个快速参考卡是为了帮助你快速查找关键信息。对于详细内容, 请查阅完整的Agent文档和使用指南。

**版本**: 2.0.0
**更新日期**: 2026-03-13

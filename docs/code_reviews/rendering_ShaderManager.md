# 代码审查报告 — ShaderManager

**文件路径**

- `include/rendering/shaders/ShaderManager.hpp`
- `src/rendering/shaders/ShaderManager.cpp`

**审查日期**: 2026-03-15
**严重程度图例**: 🔴 高风险 | 🟡 中风险 | 🟢 低风险

---

## 功能摘要

`ShaderManager` 提供着色器程序的加载、编译、缓存与热重载管理，采用 **Pimpl 惯用法**。

| 功能      | 接口                          | 实现状态                                |
|---------|-----------------------------|-------------------------------------|
| 着色器程序加载 | `load_shader_program(name)` | ⚠️ 调用 compile_shader，但 compile 为占位符 |
| 文件读取    | `load_shader(path, type)`   | ✅ 实际读取文件，调用 compile                 |
| 编译 GLSL | `compile_glsl`              | ❌ 未实现（接口仅声明）                        |
| 编译 HLSL | `compile_hlsl`              | ❌ 未实现（接口仅声明）                        |
| 热重载     | `update_hot_reloads`        | ✅ 检测文件时间戳，触发 reload                 |
| 缓存      | `enable_caching`            | ⚠️ 字段存在但未使用                         |

---

## 潜在问题

### 🔴 高风险

#### 1. `compile_shader()` 始终返回空 bytecode 并标记为成功

```cpp
ShaderCompileResult ShaderManager::compile_shader(const ShaderCompileInfo& info)
{
    // Placeholder: Actual SPIR-V compilation would go here
    result.success  = true;
    result.bytecode = std::vector<uint32_t>(); // Placeholder: 空字节码！
    return result;
}
```

**影响**：所有着色器"加载成功"，但 `bytecode` 为空。下游用 `ShaderCompileResult::bytecode` 创建 `VkShaderModule` 时会传入
`pCode = nullptr`、`codeSize = 0`，导致 Vulkan 验证层报错或驱动崩溃。

#### 2. `compile_glsl` / `compile_hlsl` 仅声明，未在 `.cpp` 中实现

头文件声明了：

```cpp
ShaderCompileResult compile_glsl(const std::string& source, ShaderType type);
ShaderCompileResult compile_hlsl(const std::string& source, ShaderType type);
```

但 `.cpp` 中完全没有这两个函数的定义。若有任何代码调用它们，会产生**链接错误**。

#### 3. `load_shader_program()` 使用文件名拼接策略，路径假设过强

```cpp
auto vertex_shader   = load_shader(name + ".vert", ShaderType::Vertex);
auto fragment_shader = load_shader(name + ".frag", ShaderType::Fragment);
```

假设程序名直接对应 `.vert` / `.frag` 文件，且该文件存在于 `shader_directory_` 下。没有配置文件或 manifest 描述程序组成，也不支持只有
Compute 着色器的程序（`geometry_shader`、`compute_shader` 字段永远为空）。

### 🟡 中风险

#### 4. `ShaderType` 与 `ResourceManager.hpp` 中的 `ShaderType` 重复定义

`ShaderManager.hpp` 和 `ResourceManager.hpp` 都在 `vulkan_engine::rendering` 命名空间中定义了完全相同的 `ShaderType`
枚举，会导致**重定义编译错误**（若两个头文件被同一翻译单元包含）。

#### 5. `update_hot_reloads()` 在路径处理上存在 stem 截断 bug

```cpp
std::string shader_name = std::filesystem::path(path).stem().string();
reload_shader(shader_name);
```

`path` 是相对路径（如 `"myshader.vert"`），`stem()` 取到 `"myshader"`，但程序可能以 `"myshader"` 注册。然而若路径包含子目录（如
`"effects/bloom.frag"`），`stem()` 只取 `"bloom"`，与注册名 `"effects/bloom"` 不匹配，导致热重载静默失败。

#### 6. 缓存功能（`enable_cache_`）虽有字段但从未使用

```cpp
struct Impl {
    bool enable_cache = true;
};
```

`compile_shader()` 中完全没有检查 `enable_cache` 或读取/写入任何缓存文件。设置此选项不会产生任何效果。

### 🟢 低风险

#### 7. `ShaderProgram` 结构体中 `geometry_shader`、`compute_shader` 字段永远为默认值

`load_shader_program()` 只加载 vertex + fragment，`geometry_shader` 和 `compute_shader`
字段无论是否存在对应文件，都不会被填充。调用方无法依赖这些字段判断着色器程序的完整性。

#### 8. `reload_all_shaders()` 先清空所有程序再逐个重新加载，无原子性保证

```cpp
impl_->programs.clear();
for (const auto& name : names) {
    load_shader_program(name);
}
```

清空后重新加载期间，若某个着色器加载失败，已成功加载的其他着色器仍然有效，但失败的着色器会在 map
中消失。整个热重载过程不是原子的，中途出错会导致部分程序丢失。

---

## 修复建议

1. **接入真正的 SPIR-V 编译器**：集成 `shaderc`（Google）或 `glslang`，替换占位符 `compile_shader()`。
2. **消除重复 `ShaderType` 枚举**：将其移至公共头文件（如 `rendering/RenderingTypes.hpp`），两处共享。
3. **修复热重载路径匹配**：在 `file_timestamps` 中存储完整程序名（而非路径 stem），或建立路径→程序名映射。
4. **`compile_glsl`/`compile_hlsl` 添加实现或删除声明**，避免链接错误。

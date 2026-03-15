# 代码审查报告：MaterialLoader

**文件路径**

- `include/rendering/material/MaterialLoader.hpp`
- `src/rendering/material/MaterialLoader.cpp`

---

## 功能概述

`MaterialLoader` 从 JSON 文件加载 `Material` 对象：

- **JSON 解析**：使用 `nlohmann/json` 解析材质配置（名称、shader 路径、render_states、parameters、textures）
- **缓存机制**：按材质名称缓存，相同名称不重复加载
- **纹理加载**：通过 `TextureLoader` 加载 albedo/normal/roughness/metallic 贴图
- **路径解析**：`resolve_path()` 尝试多个搜索路径（包括硬编码的绝对路径）

---

## 关键设计

| 特性      | 说明                                                              |
|---------|-----------------------------------------------------------------|
| JSON 格式 | nlohmann/json 解析，支持 shader/parameters/textures/render_states 节点 |
| 名称缓存    | 以 JSON 中 `name` 字段为 key，而非文件路径                                  |
| 异常安全    | 整个加载过程用 try-catch 包裹                                            |

---

## 潜在问题

### 🔴 高风险

1. **`resolve_path()` 中硬编码绝对路径 `D:/TechArt/Vulkan/`**  
   路径解析中包含硬编码的开发机绝对路径，在任何非开发者机器上运行时此路径无效，降低了代码可移植性。  
   **建议**：使用可配置的项目根路径（如从环境变量或配置文件读取）替换硬编码路径。

### 🟡 中风险

2. **缓存键为材质名称（`name` 字段），而非文件路径**  
   若两个不同路径的材质文件中 `name` 字段相同，第二次加载会直接返回缓存中的第一个材质，可能是错误的结果。  
   **建议**：使用文件路径（或路径 + render_pass 的组合）作为缓存键。

3. **`load()` 对同一材质名称不更新已缓存的结果**  
   文件修改后调用 `load()` 时，若缓存命中，不会重新解析更新后的 JSON，导致热重载无效。

### 🟢 低风险

4. **`texture_loader_` 与 `device_` 的生命周期耦合**  
   `MaterialLoader` 析构时，`texture_loader_` 持有的 `device_` 引用计数递减。若材质缓存中的 Material 还持有纹理（
   `shared_ptr<Image>`），纹理仍然存活，不会有问题，但生命周期链路较复杂。

# 代码审查报告：ObjLoader

**文件路径**

- `include/rendering/resources/ObjLoader.hpp`
- `src/rendering/resources/ObjLoader.cpp`

---

## 功能概述

自实现的 OBJ 文件加载器，支持：

- **顶点属性**：position (v)、normal (vn)、texture coord (vt)
- **面（Face）**：支持 `v`, `v/vt`, `v/vt/vn`, `v//vn` 格式，自动三角化（fan triangulation）
- **相对索引**：支持负数相对索引（OBJ 规范）
- **顶点去重**：用 `unordered_map<string, uint32_t>` 去重复合顶点
- **Windows 编码**：UTF-8 路径通过 `MultiByteToWideChar` 转换，支持中文路径

---

## 关键设计

| 特性     | 说明                            |
|--------|-------------------------------|
| 手写解析   | 无第三方依赖，逐行解析                   |
| 顶点去重   | 字符串 key 去重，保证 index buffer 正确 |
| 顶点色彩生成 | 基于坐标的 sin 函数生成顶点色彩（调试用）       |
| 调试输出   | 加载后输出前 3 个顶点和前 9 个索引          |

---

## 潜在问题

### 🔴 高风险

1. **顶点色彩用 sin 函数生成，非真实材质色彩**  
   `vertex.color = glm::vec3(0.5f + 0.5f * sin(...))` 是调试占位代码，对于真实渲染会产生不正确的颜色。加载后的 Mesh 若依赖
   `vertex.color` 字段渲染，结果不直观。

2. **调试日志每次加载都打印前 N 个顶点和索引**  
   加载大型 OBJ（百万顶点）时，这些日志虽然只打印 3 个顶点，但字符串构建开销在 debug build 下不可忽视。在 Release 构建中应完全禁用。

### 🟡 中风险

3. **Fan 三角化不适合非凸多边形**  
   Fan 三角化对凸多边形正确，但对非凸多边形（如凹面 Ngon）会产生错误的三角形，导致网格渲染错误。  
   **建议**：使用 ear-clipping 等更通用的三角化算法。

4. **`parse_face_index()` 对非法 token 使用 `std::stoi` 未捕获异常**  
   若 OBJ 文件中有格式错误的面数据，`std::stoi` 会抛出 `std::invalid_argument`，但 `load()`
   函数没有对此进行异常捕获，会导致整个加载过程失败并抛出未捕获的异常。

5. **不支持 MTL 材质文件**  
   注释中说明不支持材质，但实际上 OBJ 文件中的 `usemtl` 指令会被静默忽略，而不是给出有意义的提示。

### 🟢 低风险

6. **中文注释提到 "has_invalid" 后未对结果做过滤**  
   验证发现 NaN 或越界索引时，设置 `has_invalid = true` 并记录错误，但仍然返回包含无效数据的 `MeshData`，调用方不主动检查的话会使用错误数据。

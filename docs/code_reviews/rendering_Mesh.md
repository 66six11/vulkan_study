# 代码审查报告：Mesh

**文件路径**

- `include/rendering/resources/Mesh.hpp`
- `src/rendering/resources/Mesh.cpp`

---

## 功能概述

`Mesh` 封装 GPU 端顶点/索引缓冲，`MeshData` 为 CPU 端数据结构：

- **`MeshVertex`**：position + normal + uv + color，4 × vec3/2 = 40 字节
- **`MeshData`**：CPU 端顶点和索引数组
- **`Mesh::upload()`**：将 `MeshData` 上传到 host-visible GPU 缓冲
- **`bind()` / `draw()`**：封装 `vkCmdBindVertexBuffers` / `vkCmdDrawIndexed`

---

## 关键设计

| 特性         | 说明                             |
|------------|--------------------------------|
| 移动语义       | Mesh 支持移动，不支持拷贝                |
| Buffer 拥有权 | `unique_ptr<Buffer>` 拥有顶点/索引缓冲 |
| 高层绘制接口     | `bind()` + `draw()` 简化调用流程     |

---

## 潜在问题

### 🔴 高风险

1. **顶点缓冲使用 `HOST_VISIBLE` 而非 `DEVICE_LOCAL`**  
   `upload()` 创建的顶点缓冲和索引缓冲均使用 `HOST_VISIBLE | HOST_COHERENT` 内存，而非 device-local 内存。对于静态
   mesh，此选择导致显著的渲染性能损失（GPU 每次需从 host 内存读取顶点数据）。  
   **建议**：使用 staging buffer 上传数据，目标使用 `DEVICE_LOCAL` 内存。

### 🟡 中风险

2. **`upload()` 不检查缓冲区是否已上传**  
   重复调用 `upload()` 会直接覆盖旧缓冲区（旧的 `unique_ptr` 被新的替换，RAII 自动释放旧内存），行为正确但可能造成不必要的重上传。

3. **索引类型固定为 `VK_INDEX_TYPE_UINT32`**  
   `MeshData::indices` 为 `vector<uint32_t>`，索引类型固定，对小型 mesh（< 65536 顶点）浪费一倍内存。

4. **`MeshVertex` 结构体 layout 未添加 `[[nodiscard]]` 或对齐声明**  
   顶点数据通过 `memcpy` 直接传入 GPU，若结构体对齐不符合 shader 期望的 layout，渲染会出现错误（顶点属性绑定在 Pipeline
   中定义，需要与结构体字段顺序完全匹配）。

### 🟢 低风险

5. **`name_` 字段存储路径或名称但未区分**  
   `name_` 通过 `data.name` 赋值，而 `ObjLoader` 将完整文件路径作为 `name`，不是友好的显示名称。

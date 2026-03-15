# 代码审查报告 — ResourceManager

**文件路径**

- `include/rendering/resources/ResourceManager.hpp`
- `src/rendering/resources/ResourceManager.cpp`

**审查日期**: 2026-03-15
**严重程度图例**: 🔴 高风险 | 🟡 中风险 | 🟢 低风险

---

## 功能摘要

`ResourceManager` 是渲染系统的统一资源管理门面类，采用 **Pimpl 惯用法**将实现细节隐藏在 `Impl` 结构中。负责管理四类资源的生命周期：

| 资源类型               | 存储容器                                          | 操作接口                              |
|--------------------|-----------------------------------------------|-----------------------------------|
| `Mesh`             | `unordered_map<ResourceID, Mesh>`             | load / unload / get / is_loaded   |
| `Texture`          | `unordered_map<ResourceID, Texture>`          | load / unload / get / is_loaded   |
| `MaterialResource` | `unordered_map<ResourceID, MaterialResource>` | create / unload / get / is_loaded |
| `Shader`           | `unordered_map<ResourceID, Shader>`           | load / unload / get / is_loaded   |

同时声明了异步加载队列 (`load_async` / `wait_for_all_loads`) 和热重载机制 (`update_hot_reloads` /
`enable_hot_reloading`)。

资源 ID 由 `next_id` 单调递增分配，`INVALID_RESOURCE_ID = 0` 作为无效标记。

---

## 关键设计

```
ResourceManager
  └── Impl
        ├── unordered_map<ResourceID, Mesh>             meshes
        ├── unordered_map<ResourceID, Texture>          textures
        ├── unordered_map<ResourceID, MaterialResource> materials
        ├── unordered_map<ResourceID, Shader>           shaders
        ├── ResourceID next_id = 1
        ├── queue<function<void()>> load_queue      ← 仅入队，从不消费
        └── mutex queue_mutex
```

---

## 潜在问题

### 🔴 高风险

#### 1. 所有 `load_*` 方法均为空实现（占位符）

```cpp
ResourceID ResourceManager::load_mesh(const std::filesystem::path& path, const MeshLoadOptions& /*options*/)
{
    auto id = impl_->next_id++;
    // Placeholder: Actual mesh loading implementation would go here
    Mesh mesh;
    mesh.id           = id;
    mesh.name         = path.filename().string();
    impl_->meshes[id] = std::move(mesh);
    return id;  // 返回 ID 但 mesh 完全没有顶点/索引数据
}
```

**影响**：调用方拿到合法 ID，但 `get_mesh()` 返回的 `Mesh*` 中没有任何 GPU 资源（`vertex_buffer_`, `index_buffer_`
均为空）。直接使用会导致空指针解引用或渲染崩溃。

#### 2. `load_async()` 任务队列从未被消费

```cpp
void ResourceManager::load_async(std::function<void()> load_func) {
    std::lock_guard<std::mutex> lock(impl_->queue_mutex);
    impl_->load_queue.push(std::move(load_func));
}

void ResourceManager::wait_for_all_loads() {
    // Placeholder: Wait for all async loading to complete
}
```

**影响**：提交的任务永远不会执行，`wait_for_all_loads()` 也不会等待任何东西。调用方以为异步加载完成，实际上资源从未加载。

#### 3. `register_mesh_loader` / `register_texture_loader` 完全无效

```cpp
void ResourceManager::register_mesh_loader(const std::string& /*extension*/, MeshLoader /*loader*/) {
    // Placeholder: Mesh loader registry
}
```

**影响**：自定义扩展名加载器注册后被忽略，`load_mesh()` 无论如何都走占位符路径。

### 🟡 中风险

#### 4. `ResourceID` 类型为 `uint32_t`，无类型标签区分资源类别

`load_mesh()` 和 `load_texture()` 都返回 `ResourceID`，但 `get_mesh(texture_id)` 和 `get_texture(mesh_id)` 不会报错——会直接查找并返回
`nullptr`，导致调用方只能通过 `nullptr` 检查来隐式发现错误，无法在编译期捕获类型混用。

#### 5. `update_hot_reloads()` 为空实现

```cpp
void ResourceManager::update_hot_reloads() {
    // Placeholder: Check for modified shaders and reload them
}
```

每帧调用此函数不会产生任何效果，但也不会报告错误，让调用方误以为热重载正在工作。

#### 6. 共享 `next_id` 跨越四种资源类型，有语义歧义

所有资源类型共用同一个 `next_id` 计数器。删除一个 Mesh 并重新加载时，ID 不会复用，理论上 `uint32_t`
会在大量加载卸载后溢出（虽然实践中不太可能触发，但 `erase` 后 ID 永久失效也意味着 ID 空间浪费）。

### 🟢 低风险

#### 7. 头文件中 `Mesh`、`Texture` 等结构体与 `src/rendering/resources/Mesh.hpp` 同名类冲突

`ResourceManager.hpp` 自己定义了一个只有 `id` 和 `name` 的 `Mesh` 结构，而 `rendering/resources/Mesh.hpp` 中有完整的
`Mesh` 类（含 GPU 缓冲区）。两者在同一 `vulkan_engine::rendering` 命名空间下，会造成命名歧义或编译错误。

#### 8. `enable_hot_reloading(bool /*enable*/)` 实现中不存储状态

```cpp
void ResourceManager::enable_hot_reloading(bool /*enable*/) {
    // Placeholder: Enable/disable hot reloading
}
```

调用后 `Impl` 中没有对应字段更新，调用方无法通过任何 API 查询当前热重载是否已启用。

---

## 修复建议

1. **优先实现真正的加载路径**，或明确将接口标记为 `[[deprecated]]` / 添加 `assert(false)` 避免无声失败。
2. **异步队列**需要配套工作线程（`std::thread` 或 `std::async`）和消费逻辑。
3. **区分资源类型的 ID**：考虑使用强类型 ID（如 `MeshID`, `TextureID`），或在 `ResourceID` 中编码类型前缀位。
4. **删除头文件中的冗余结构体**，直接引用 `rendering/resources/Mesh.hpp` 中的完整类型。

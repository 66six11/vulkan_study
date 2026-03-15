# 代码审查报告 — Scene

**文件路径**

- `include/rendering/scene/Scene.hpp`
- `src/rendering/scene/Scene.cpp`

**审查日期**: 2026-03-15
**严重程度图例**: 🔴 高风险 | 🟡 中风险 | 🟢 低风险

---

## 功能摘要

`Scene` 是渲染引擎的场景容器，管理一组 `Entity` 的生命周期，并提供空间查询接口。采用 **Pimpl 惯用法**隐藏实现细节。

| 功能模块   | 接口                                                | 实现状态                      |
|--------|---------------------------------------------------|---------------------------|
| 实体管理   | `create_entity` / `destroy_entity` / `get_entity` | ✅ 基本实现                    |
| 场景生命周期 | `load` / `unload` / `is_loaded`                   | ⚠️ 占位符                    |
| 更新与渲染  | `update(delta_time)` / `render(context)`          | ❌ 空实现                     |
| 空间索引   | `SpatialIndex` 抽象接口 + 委托                          | ⚠️ 无默认实现                  |
| 视锥裁剪   | `frustum_cull`                                    | ⚠️ 无 SpatialIndex 时返回全部实体 |
| 射线检测   | `raycast`                                         | ❌ 暴力搜索路径为空                |

---

## 潜在问题

### 🔴 高风险

#### 1. `destroy_entity()` 使实体指针悬空（使用后释放）

```cpp
void Scene::destroy_entity(Entity* entity)
{
    // 先从 map 中删除
    impl_->entity_map.erase(entity->name);
    // 再从 vector 中 erase → unique_ptr 析构，entity 指针变悬空
    impl_->entities.erase(entity_it);
    // 此后任何持有 entity 的地方都是悬空指针
}
```

**影响**：外部代码持有 `Entity*` 时，调用 `destroy_entity()` 后会产生悬空指针，访问该指针导致 UB。应通过 `EntityID` 或
`weak_ptr` 代替裸指针访问实体。

#### 2. `Entity::id` 直接使用 `entities.size()` 分配，删除后 ID 混乱

```cpp
entity->id = static_cast<uint32_t>(impl_->entities.size());
```

删除实体后，再创建新实体会复用已被释放实体的 ID（因为 `vector.size()` 减小了）。同名实体也会覆盖 `entity_map` 中的旧条目但不清理旧
`Entity` 对象，导致多实体同名时只能找到后添加的那个。

#### 3. `update()` 和 `render()` 完全为空

```cpp
void Scene::update(float /*delta_time*/) {
    for (auto& entity : impl_->entities) {
        (void)entity;  // 什么都不做
    }
}
void Scene::render(RenderContext& /*context*/) {
    for (auto& entity : impl_->entities) {
        (void)entity;  // 什么都不做
    }
}
```

**影响**：场景系统无法驱动实体的任何行为更新和渲染。这是 MVP 占位符级别的代码，不能用于实际渲染。

### 🟡 中风险

#### 4. `RaycastResult` 持有裸 `Entity*`，生命周期不安全

```cpp
struct RaycastResult {
    Entity* entity = nullptr;  // 裸指针
};
```

射线检测结果中的 `entity` 指针在使用期间可能已被 `destroy_entity()` 释放。应改为返回 `EntityID` 再通过 `get_entity()`
查询，或使用 `weak_ptr`。

#### 5. `frustum_cull()` 无 SpatialIndex 时退化为返回全部实体

```cpp
// No spatial indexing, return all entities
for (auto& entity : impl_->entities) {
    out_visible.push_back(entity.get());
}
```

在没有安装 `SpatialIndex` 的情况下，所有实体都被认为可见，完全没有裁剪效果。对大场景性能影响极大，且没有警告日志提示用户当前未启用裁剪。

#### 6. `SpatialIndex` 接口设计过于简化

`Frustum`、`Ray` 结构体完全为空（只有注释）：

```cpp
struct Frustum { /* Frustum planes for culling */ };
struct Ray { /* Origin and direction */ };
```

任何实现 `SpatialIndex` 的类无法真正使用这些结构来做空间计算。`frustum_cull` 和 `raycast` 的接口在 `Frustum`/`Ray`
填充真实数据之前都是无意义的。

### 🟢 低风险

#### 7. `entity_map` 键为实体名称（字符串），不支持同名实体

```cpp
impl_->entity_map[name] = ptr;
```

创建同名实体时会默默覆盖 `entity_map` 中的条目，但 `entities`
中仍然保留两个对象，导致通过名称只能找到后创建的同名实体，前一个实体"丢失"但内存未释放。

#### 8. `load()` 实现仅设置 `is_loaded = true`，无任何实际加载逻辑

```cpp
bool Scene::load() {
    impl_->is_loaded = true;
    return true;
}
```

没有从文件系统或数据库加载场景数据的逻辑，但 `is_loaded()` 返回 `true` 可能让调用方误以为场景已就绪。

---

## 修复建议

1. **实体 ID 管理**：使用独立的单调递增计数器（`uint32_t next_id_`），不依赖 `vector.size()`。
2. **避免裸指针**：考虑用 `EntityID`（整型）作为外部句柄，`get_entity(EntityID)` 返回 `Entity*`，并在 `destroy_entity` 中通过
   ID 而非指针操作。
3. **`Entity` 组件系统**：`Entity` 结构体目前只有 `id` 和 `name`，需要添加 Transform、MeshRenderer 等组件才能支持 `update`/
   `render`。
4. **`Frustum`/`Ray` 补全**：至少包含平面法线/点和射线原点/方向字段，使 SpatialIndex 实现可以真正工作。

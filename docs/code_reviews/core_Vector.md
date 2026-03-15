# 代码审查报告：Vector

**文件路径**

- `include/core/math/Vector.hpp`

---

## 功能概述

自定义模板向量类 `Vector<T, N>`，提供：

- **基础运算**：加减、标量乘除、点积、叉积（3D）
- **工具函数**：`length()`, `normalized()`, `dot()`, `cross()`
- **C++20 Concepts**：通过 `Arithmetic` concept 约束模板参数
- **常用别名**：`Vec2f`, `Vec3f`, `Vec4f`, `Vec2i` 等

---

## 关键设计

| 特性             | 说明                                      |
|----------------|-----------------------------------------|
| 模板化            | 支持任意数值类型和维度                             |
| C++20 Concepts | `requires std::is_arithmetic_v<T>` 静态检查 |
| 与 GLM 互操作      | 提供到 `glm::vec` 的隐式转换                    |

---

## 潜在问题

### 🟡 中风险

1. **缺少标量乘法 `operator*`（向量左乘）**  
   仅实现了 `Vec * scalar`，未实现 `scalar * Vec` 形式。在数学代码中，`2.0f * v` 的写法无法通过编译，与 GLM 行为不一致。  
   **建议**：添加 `friend Vector operator*(T scalar, const Vector& v)`。

2. **`normalized()` 在零向量时行为未文档化**  
   零向量调用 `normalized()` 时直接返回原向量（长度为 0），而不是返回零向量或抛出异常，可能导致后续计算出现 NaN。  
   **建议**：添加注释说明行为，或添加断言/返回错误状态。

### 🟢 低风险

3. **与项目实际使用的 GLM 重复**  
   项目中大量使用 `glm::vec3` 等 GLM 类型，自定义 `Vector` 类使用较少，维护两套类型系统增加了复杂度。  
   **建议**：评估是否有必要保留自定义 Vector，或者统一使用 GLM。

4. **`cross()` 只对 3D 有意义**  
   `cross()` 方法在 `Vector<T, 2>` 上也能编译（返回 `Vector<T, 3>`），但语义不清晰。

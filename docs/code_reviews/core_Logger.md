# 代码审查报告：Logger

**文件路径**

- `include/core/utils/Logger.hpp`

---

## 功能概述

极简日志工具，提供 `logger::info`, `logger::warn`, `logger::error`, `logger::debug` 四个函数，所有实现均为纯 `inline`
函数，直接输出到 `stdout`/`stderr`。

---

## 关键设计

| 特性    | 说明                                  |
|-------|-------------------------------------|
| 零依赖   | 仅使用标准 `<iostream>`                  |
| 跨平台颜色 | 使用 ANSI 转义码着色（在 Windows CMD 下可能不生效） |
| 简单易用  | 单参数字符串接口，无格式化                       |

---

## 潜在问题

### 🔴 高风险

1. **无日志级别过滤**  
   所有 `logger::info` 调用在所有构建配置下都会输出。项目中大量的调试日志（尤其是 `CubeRenderPass` 每帧的 info
   输出）会在发布版本中产生严重性能开销。  
   **建议**：添加全局日志级别控制，Release 构建下过滤 `info` 和 `debug`。

2. **无时间戳**  
   日志无时间信息，无法判断问题发生的时间顺序，对调试线程竞争和性能问题帮助有限。

### 🟡 中风险

3. **无文件输出**  
   没有将日志写入文件的能力，调试崩溃问题时需要重现场景。

4. **`std::cout` 无缓冲控制**  
   频繁调用 `std::cout << ... << std::endl` 会因每次 flush 而造成性能损失，大量日志输出时尤为明显。  
   **建议**：使用 `'\n'` 替代 `std::endl`，或改为行缓冲策略。

### 🟢 低风险

5. **Windows 控制台颜色兼容性**  
   ANSI 转义码在旧版 Windows 控制台（非 Windows Terminal）下不生效，建议添加 `ENABLE_VIRTUAL_TERMINAL_PROCESSING` 初始化，或使用
   `SetConsoleTextAttribute` 作为 fallback。

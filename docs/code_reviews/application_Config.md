# 代码审查报告：Config

**文件路径**

- `include/application/config/Config.hpp`
- `src/application/config/Config.cpp`

---

## 功能概述

简单的 INI 风格配置文件读写模块，支持：

- **读取**：从 `key=value` 文本文件解析键值对
- **写入**：将配置序列化回文件
- **类型访问**：`get_string`, `get_int`, `get_float`, `get_bool`
- **文件信息查询**：`get_file_info()` 返回路径、权限、修改时间等

---

## 关键设计

| 特性           | 说明                                                      |
|--------------|---------------------------------------------------------|
| 简单键值存储       | 内部使用 `std::unordered_map<string, string>`               |
| 类型转换         | `get_int/float/bool` 使用 `std::stoi/stof` + 默认值 fallback |
| 注释支持         | 忽略 `#` 开头的行                                             |
| 节（Section）支持 | 可选，用 `[section]` 区分命名空间                                 |

---

## 潜在问题

### 🔴 高风险

1. **`is_readable` / `is_writable` 硬编码为 `true`**  
   `get_file_info()` 中返回的 `FileInfo` 结构体中，`is_readable` 和 `is_writable` 始终返回 `true`
   ，未实际检查文件系统权限。在只读目录中尝试写入会在运行时静默失败。  
   **建议**：使用 `std::filesystem::status()` 检查实际权限。

2. **`creation_time` 实际返回的是修改时间**  
   `get_file_info()` 中 `creation_time` 字段使用 `std::filesystem::last_write_time()` 填充，语义与名称不符。  
   **建议**：重命名字段或使用平台 API 获取真实创建时间。

### 🟡 中风险

3. **`get_int` / `get_float` 异常处理不全**  
   当 value 字符串格式非法时，`std::stoi` / `std::stof` 会抛出异常，但调用方未必能处理。  
   **建议**：捕获 `std::invalid_argument` / `std::out_of_range` 并返回默认值。

### 🟢 低风险

4. **无多线程保护**  
   并发读写 `config_map_` 时缺少互斥锁，若未来多线程热重载配置会产生数据竞争。

# 一键添加BOM编码工具使用指南

## 工具说明

已为项目创建了以下工具来一键添加UTF-8 BOM编码：

### 工具列表

1. **add_bom.py** - Python主脚本
2. **add_bom.bat** - Windows批处理脚本
3. **add_bom.sh** - Linux/macOS Shell脚本

## UTF-8 BOM说明

UTF-8 BOM (Byte Order Mark) 是3个字节：`EF BB BF`

**作用**：

- 明确标识文件编码为UTF-8
- 在Windows平台上更好地兼容某些编辑器和工具
- 防止编码识别错误

**注意**：

- 某些Unix/Linux工具可能不推荐使用BOM
- Shell脚本通常不需要BOM
- 现代编辑器通常能自动识别UTF-8编码

## 使用方法

### Windows

```batch
# 添加BOM到所有代码文件
add_bom.bat

# 检查哪些文件需要BOM
add_bom.bat --check

# 删除所有文件的BOM
add_bom.bat --remove

# 预览模式（不实际修改文件）
add_bom.bat --dry-run
```

### Linux/macOS

```bash
# 添加权限
chmod +x add_bom.sh

# 添加BOM到所有代码文件
./add_bom.sh

# 检查哪些文件需要BOM
./add_bom.sh --check

# 删除所有文件的BOM
./add_bom.sh --remove

# 预览模式
./add_bom.sh --dry-run
```

### 直接使用Python

```bash
# 添加BOM
python add_bom.py --verbose

# 检查文件
python add_bom.py --check

# 删除BOM
python add_bom.py --remove --verbose

# 预览模式
python add_bom.py --dry-run

# 指定目录
python add_bom.py --dir /path/to/project
```

## 处理的文件类型

工具会自动处理以下类型的文件：

**C/C++文件**：

- `.cpp`, `.hpp`, `.c`, `.h`, `.cxx`, `.hxx`

**CMake文件**：

- `.cmake`, `.txt` (CMakeLists.txt等)

**Python文件**：

- `.py`

**脚本文件**：

- `.sh`, `.bat`

**文档文件**：

- `.md`

**配置文件**：

- `.json`

**着色器文件**：

- `.vert`, `.frag`, `.geom`, `.comp`, `.tesc`, `.tese`

## 排除的目录

以下目录会被自动排除：

- `build/`, `build_vs/`, `build_test/`, `build_conan/`
- `.git/`, `.vs/`, `.vscode/`
- `CMakeFiles/`, `__pycache__/`

## 执行结果

运行后发现：**所有代码文件已经有UTF-8 BOM**

这是因为使用WorkBuddy的`write_to_file`工具时，系统自动为所有新建文件添加了UTF-8 BOM编码。

## 验证结果

```bash
python add_bom.py --check
```

输出：

```
Scanning directory: D:\TechArt\Vulkan

Found 62 code files
All files already have BOM!
```

## 建议

### 对于Windows开发者

- ✅ 推荐使用UTF-8 BOM
- ✅ 与Visual Studio、VS Code等编辑器兼容性好
- ✅ 避免编码识别问题

### 对于跨平台项目

- ⚠️ 需要权衡Windows和Unix/Linux平台的习惯
- ⚠️ 某些CI/CD工具可能对BOM有特殊要求
- ⚠️ Shell脚本通常不需要BOM

### 对于当前项目

当前项目已经全部使用UTF-8 BOM编码，建议保持现状，因为：

1. 项目主要在Windows平台开发
2. 使用Visual Studio构建
3. BOM不会影响CMake和编译器处理
4. 与团队协作工具兼容性更好

## 相关配置

### Git配置

可以在`.gitattributes`中配置编码：

```
# Auto detect text files and perform LF normalization
* text=auto

# Source code
*.cpp text eol=lf
*.hpp text eol=lf
*.c text eol=lf
*.h text eol=lf

# CMake
*.cmake text eol=lf
CMakeLists.txt text eol=lf

# Python
*.py text eol=lf

# Shell scripts
*.sh text eol=lf

# Batch files
*.bat text eol=crlf

# Markdown
*.md text eol=lf

# JSON
*.json text eol=lf
```

### EditorConfig

可以创建`.editorconfig`文件：

```ini
root = true

[*]
charset = utf-8-bom
end_of_line = lf
insert_final_newline = true
trim_trailing_whitespace = true

[*.bat]
end_of_line = crlf

[*.{cpp,hpp,c,h}]
indent_style = space
indent_size = 4

[*.py]
indent_style = space
indent_size = 4

[CMakeLists.txt]
indent_style = space
indent_size = 2

[*.cmake]
indent_style = space
indent_size = 2
```

## 故障排除

### 问题1：Python未安装

```
[ERROR] Python not found!
```

**解决**：安装Python 3.6或更高版本

### 问题2：权限不足

```
[ERROR] Permission denied
```

**解决**：

- Windows: 以管理员身份运行
- Linux/macOS: `chmod +x add_bom.sh`

### 问题3：文件被占用

```
[ERROR] File in use
```

**解决**：关闭所有编辑器和IDE，重新运行

## 总结

✅ 已创建完整的BOM管理工具
✅ 所有代码文件已有UTF-8 BOM编码
✅ 工具可用于未来添加/删除/检查BOM
✅ 支持Windows、Linux、macOS多平台

---

**创建日期**: 2026年3月13日

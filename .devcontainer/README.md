# IRMV IMC 开发容器配置

这个 `.devcontainer` 配置为 IRMV Manipulation Core (IMC) 项目提供了一个完整的开发环境。

## 文件说明

- `devcontainer.json` - VS Code Dev Container 主配置文件
- `docker-compose.yml` - Docker Compose 配置（已优化网络环境）
- `README.md` - 本说明文档

## 🌐 网络环境优化

本配置已针对中国大陆网络环境进行优化：

### 🔧 **已解决的问题：**

1. **GitHub Container Registry 访问问题** - 已移除外部 features 依赖
2. **DNS 解析问题** - 使用中国大陆友好的 DNS 服务器
3. **Python 包下载优化** - 使用清华大学 PyPI 镜像源

### 📝 **配置特性：**

- 配置了国内 DNS 服务器（114.114.114.114, 223.5.5.5）
- 使用清华 PyPI 镜像源
- 优化网络模式以支持调试工具

## 使用方法

### 方法 1: 使用 VS Code Dev Containers（推荐）

1. 确保已安装 [Remote - Containers](https://marketplace.visualstudio.com/items?itemName=ms-vscode-remote.remote-containers) 扩展
2. 在 VS Code 中打开项目根目录
3. 按 `Ctrl+Shift+P` 打开命令面板
4. 选择 `Dev Containers: Reopen in Container`
5. VS Code 会自动构建并启动开发容器

### 方法 2: 使用 Docker Compose 命令行

```bash
# 启动开发环境
cd .devcontainer
docker-compose up -d imc-dev

# 进入容器
docker-compose exec imc-dev bash

# 停止环境
docker-compose down
```

### 方法 3: 运行构建测试

```bash
# 使用构建服务进行完整测试和基准测试
docker-compose --profile build up imc-builder
```

## 功能特性

### 🔧 **开发工具**
- **调试工具**: gdb, valgrind, strace
- **网络工具**: ping, net-tools
- **Python 开发**: pytest, black, flake8
- **C++ 开发**: CMake, gcc/g++, OMPL

### 📁 **卷挂载**
- 源码目录: `../` → `/workspace`
- 构建缓存: `build-cache` 卷用于加速构建
- VS Code 扩展持久化

### 🌐 **端口转发**
- `8080`: 用于开发服务器
- `3000`: 用于调试端口

### 🔑 **权限配置**
- 启用了 `SYS_PTRACE` 和 `SYS_ADMIN` 能力
- 支持调试器和性能分析工具

## 环境变量

| 变量名 | 值 | 说明 |
|--------|-----|------|
| `CC` | gcc | C 编译器 |
| `CXX` | g++ | C++ 编译器 |
| `CMAKE_BUILD_TYPE` | Debug | CMake 构建类型 |
| `PYTHON_VERSION` | 3.10 | Python 版本 |

## VS Code 扩展

已预配置以下扩展：
- **C++ 开发**: C/C++, CMake Tools, Makefile Tools, Clangd
- **Python 开发**: Python, Pylint, Black Formatter
- **版本控制**: GitLens, GitHub Pull Requests
- **其他工具**: YAML, Code Spell Checker

## 故障排除

### 🌐 Docker Compose 相关问题

**问题**: `Command failed: docker compose build`
```bash
# 解决方案：检查 Dockerfile.dev 中的包名
# 常见问题：libboost_serialization-dev 不存在，应使用 libboost-all-dev
cd .devcontainer
docker compose build --no-cache imc-dev
```

**问题**: `Command failed: docker compose up -d`
```bash
# 解决方案：检查并修复配置
cd .devcontainer
docker compose config  # 验证配置
docker compose down --remove-orphans  # 清理旧容器
docker compose up -d imc-dev
```

**问题**: 容器名称冲突
```bash
# 强制删除冲突的容器
docker rm -f imc-development
docker compose up -d imc-dev
```

**问题**: DNS 解析失败
```bash
# 手动设置 DNS
docker compose exec imc-dev bash
echo "8.8.8.8" > /etc/resolv.conf
```

### 🐛 容器启动失败
```bash
# 清理并重建
docker compose down -v
docker compose build --no-cache imc-dev
docker compose up -d imc-dev
```

### 权限问题
```bash
# 检查用户权限
docker compose exec imc-dev whoami
docker compose exec imc-dev id
```

### 构建缓存问题
```bash
# 清理构建缓存卷
docker volume rm devcontainer_vscode-extensions
docker compose down -v
```

## 自定义配置

### 添加环境变量
在 `.env` 文件中设置：
```bash
CMAKE_BUILD_TYPE=Release
ENABLE_CUDA=ON
```

### 修改挂载点
编辑 `docker-compose.yml` 中的 `volumes` 部分：
```yaml
volumes:
  - /path/to/local:/path/to/container:rw
```

### 添加新服务
在 `docker-compose.yml` 中添加新服务定义并使用 `profiles` 进行分组管理。

## 性能优化

1. **使用构建缓存**: `build-cache` 卷会保留 CMake 缓存
2. **选择构建类型**: Debug 用于开发，Release 用于测试
3. **并行构建**: CMake 默认启用并行构建
4. **增量构建**: 跳过未更改的组件

## 安全注意事项

- 容器以 `irmv` 用户运行，无需 root 权限
- 只在必要时启用特权模式用于调试
- 定期更新基础镜像以获得安全补丁

---

**维护者**: IRMV Robot Manipulation Group  
**最后更新**: 2024年


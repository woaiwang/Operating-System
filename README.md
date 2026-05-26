# FAT文件系统模拟程序

## 项目简介

本项目是一个模拟 DOS FAT 文件系统的实验项目，实现了基本的文件系统功能，包括：
- 用户登录/注销
- 目录管理（创建、切换、显示）
- 文件管理（创建、删除、打开、关闭、读写）
- 磁盘格式化
- 系统停机

## 项目结构

```
filesystem-project/
├── include/
│   └── filesys.h          # 头文件（常量、数据结构、接口声明）
├── src/
│   ├── main.c             # 主程序（交互式命令解释器）
│   ├── block.c            # 块管理（balloc/bfree）
│   ├── inode.c            # inode管理（iget/iput/ialloc/ifree）
│   ├── format.c           # 磁盘格式化
│   ├── install.c          # 文件系统安装
│   ├── halt.c             # 系统停机
│   ├── name.c             # 文件名解析（namei/iname）
│   ├── access.c           # 权限检查
│   ├── dir.c              # 目录操作（_dir/mkdir/chdir）
│   ├── creat.c            # 文件创建
│   ├── open.c             # 文件打开
│   ├── close.c            # 文件关闭
│   ├── rdwt.c             # 文件读写
│   ├── delete.c           # 文件删除
│   └── log.c              # 用户登录/注销
├── Makefile               # Linux/Mac编译脚本
├── build.bat              # Windows编译脚本
└── README.md              # 项目说明
```

## 编译运行

### Windows 系统
```cmd
build.bat
build\fs.exe
```

### Linux/Mac 系统
```bash
make
./build/fs
```

## 可用命令

| 命令 | 功能 |
|------|------|
| `login <uid> <password>` | 用户登录（默认用户：uid=2118, password=abcd） |
| `logout` | 用户登出 |
| `dir` | 显示目录内容 |
| `mkdir <dirname>` | 创建目录 |
| `chdir <dirname>` | 切换目录 |
| `creat <filename> <mode>` | 创建文件 |
| `delete <filename>` | 删除文件 |
| `open <filename> <mode>` | 打开文件 |
| `close <fd>` | 关闭文件 |
| `read <fd> <size>` | 读取文件 |
| `write <fd> <size>` | 写入文件 |
| `format` | 格式化磁盘 |
| `halt` | 退出系统 |
| `help` | 显示帮助 |
| `exit` | 退出程序 |

## 测试流程

1. 启动程序后，系统会自动安装文件系统
2. 使用 `login 2118 abcd` 登录系统
3. 使用 `dir` 查看当前目录
4. 使用 `mkdir <dirname>` 创建目录
5. 使用 `chdir <dirname>` 切换目录
6. 使用 `creat <filename> 01777` 创建文件
7. 使用 `open <filename> 2` 打开文件（模式2为读写）
8. 使用 `write <fd> <size>` 写入数据
9. 使用 `close <fd>` 关闭文件
10. 使用 `halt` 退出系统

## 文件系统结构

- **块大小**: 512字节
- **磁盘大小**: 512块（256KB）
- **inode数量**: 512个
- **最大目录项**: 128个
- **用户数量**: 10个
- **系统打开文件数**: 40个
- **用户打开文件数**: 20个

## 默认用户

| 用户ID | 密码 | 用户组 |
|--------|------|--------|
| 2118 | abcd | 1 |

## 注意事项

1. 首次运行时如果没有 `filesystem` 文件，系统会自动格式化
2. 格式化会清除所有数据，请谨慎操作
3. 文件系统数据保存在当前目录的 `filesystem` 文件中
4. 建议在运行前备份 `filesystem` 文件
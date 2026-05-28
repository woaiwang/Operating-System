# Operating-System — Unix V6 风格文件系统模拟器

操作系统课程项目。用 C99 实现一个完整的 Unix V6 风格文件系统，包含块分配、inode 管理、文件/目录操作和交互式 Shell。

## 快速开始

### 环境要求

- **Windows** + **MSYS2 UCRT64** 终端
- GCC（C99）+ GNU Make

### 安装工具链

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-make
ln -sf /ucrt64/bin/mingw32-make.exe /ucrt64/bin/make.exe
```

### 编译 & 运行

```bash
# 编译
make clean && make

# 跑测试（22 项全部 PASS）
./build/fs.exe --test

# 进入交互式 Shell
./build/fs.exe

# 直接用 gcc（不需要 make）
gcc -std=c99 -Wall -Wextra -Iinclude src/*.c -o build/fs.exe
```

### 调试模式

编译时加 `-DFS_DEBUG` 开启调试输出：

```bash
gcc -std=c99 -Wall -Wextra -DFS_DEBUG -Iinclude src/*.c -o build/fs.exe
```

---

## 交互式 Shell 使用

启动后输入命令操作文件系统：

```
fs> format                # 创建新的文件系统
fs> dir                   # 列出当前目录
fs> mkdir testdir         # 创建子目录
fs> chdir testdir         # 切换目录
fs> creat myfile          # 创建文件
fs> write 1 "hello"       # 写入文件
fs> read 1 5              # 读取文件
fs> close 1               # 关闭文件
fs> delete myfile         # 删除文件
fs> halt                  # 退出
```

完整命令列表：`help`, `format`, `install`, `halt`, `dir`, `mkdir`, `chdir`, `creat`, `open`, `close`, `read`, `write`, `delete`, `login`, `logout`

---

## 项目结构

```
Operating-System/
├── include/
│   └── filesys.h         # 所有结构体、常量、函数声明
├── src/
│   ├── main.c            # 入口（--test 测试模式 / 交互 Shell）
│   ├── test.c            # 自测代码（22 项）
│   ├── globals.c         # 全局变量定义
│   ├── block.c           # 块分配/释放/读写（V6 链式空闲块）
│   ├── inode.c           # inode 分配/释放/iget/iput（哈希链）
│   ├── format.c          # 创建磁盘镜像 + 初始化根目录
│   ├── install.c         # 挂载已有磁盘镜像
│   ├── halt.c            # 写回超级块 + 关闭镜像
│   ├── name.c            # 目录中按名查找/存储（namei/iname）
│   ├── access.c          # 权限检查（user → group → other）
│   ├── open.c            # 打开文件（aopen）
│   ├── close.c           # 关闭文件
│   ├── creat.c           # 创建文件
│   ├── delete.c          # 删除文件
│   ├── dir.c             # 列目录、mkdir、chdir
│   ├── rdwt.c            # 文件读写（fs_read/fs_write）
│   ├── log.c             # 用户登录/注销
│   └── file.c            # 交互式命令 Shell
├── orig/                 # 原始教师参考代码（不编译，有 30 处 bug）
├── build/
│   └── filesystem.img    # 磁盘镜像（运行时生成）
├── Makefile
├── .gitignore
└── README.md
```

---

## 磁盘布局

```
绝对块 0          → 引导块（未使用）
绝对块 1          → 超级块 (struct filsys)
绝对块 2..33      → inode 表（32 块，每块 16 个 dinode，共 512 个）
绝对块 34..545    → 数据区（512 块，每块 512 字节）
```

- `DATASTART = 17408`（字节偏移，指向第一个数据块）
- `s_free[]` 和 `di_addr[]` 存的是**数据块索引**（0..511），不 是绝对块号
- `bread()`/`bwrite()` 吃的是**绝对块号**
- 磁盘镜像固定大小：279,552 字节（546 块 × 512）

---

## FD（文件描述符）约定

| 函数 | 返回类型 | 失败值 | 有效值 |
|------|----------|--------|--------|
| `creat()` | `int` | -1 | fd ≥ 1 |
| `aopen()` | `unsigned short` | 0 | fd ≥ 1 |
| `close()` / `fs_read()` / `fs_write()` | — | — | 接受 1-based fd |

`user[].u_ofile[0]` 故意留空，让 fd=0 明确表示失败。

---

## 测试结果（22 项全 PASS）

```
=== smoke test start ===
bread/bwrite test:             PASS
balloc test #1:                PASS (block=511)
balloc test #2:                PASS (block=510, prev=511)
balloc/bfree roundtrip:        PASS (block=510, expected=510)
iget/iput test:                PASS
ialloc test #1:                PASS
ialloc/ifree roundtrip:        PASS (i_ino=4, prev=4)
ialloc sequential:             PASS (i_ino=5, prev=4)

=== integration test start ===
root dir init:                 PASS
iname test:                    PASS (idx=2)
namei test:                    PASS (ino=2)
namei not-found:               PASS
creat test:                    PASS (fd=1)
fs_write test:                 PASS (wrote 18 bytes)
fs_read test:                  PASS (read 18 bytes: "Hello, filesystem!")
close test:                    PASS
aopen+read test:               PASS (read "Hello,")
mkdir test:                    PASS (ino=5)
mkdir is-dir:                  PASS
chdir test:                    PASS
delete test:                   PASS
login bad-pw:                  PASS
login good:                    PASS (user_id=0, uid=1)
logout test:                   PASS
```

---

## 模块完成状态

| 层级 | 文件 | 功能 | 状态 |
|------|------|------|------|
| A 层 | `block.c` | 块分配/释放/读写 | ✅ |
| A 层 | `inode.c` | inode 分配/释放/iget/iput | ✅ |
| A 层 | `format.c` | 创建磁盘镜像 + 根目录初始化 | ✅ |
| A 层 | `install.c` | 挂载已有磁盘镜像 | ✅ |
| A 层 | `halt.c` | 写回超级块、关闭镜像 | ✅ |
| B 层 | `name.c` | 目录按名查找/存储 | ✅ |
| B 层 | `access.c` | 权限检查 | ✅ |
| B 层 | `open.c` | 打开文件 | ✅ |
| B 层 | `close.c` | 关闭文件 | ✅ |
| B 层 | `creat.c` | 创建文件 | ✅ |
| B 层 | `delete.c` | 删除文件 | ✅ |
| B 层 | `dir.c` | 列目录、mkdir、chdir | ✅ |
| B 层 | `rdwt.c` | 文件读写 | ✅ |
| B 层 | `log.c` | 用户登录/注销 | ✅ |
| Shell | `file.c` | 交互式命令 Shell | ✅ |
| 入口 | `main.c` | 测试模式 + 交互模式切换 | ✅ |

---

## 开发规范

- **分支策略**：功能分支 + PR，禁止直接 push 到 master
- **提交粒度**：每次只做一件事，每个 commit 必须能编译
- **提交格式**：`<type>: <简短描述>`
  - `feat:` — 新功能
  - `fix:` — 修 bug
  - `refactor:` — 重构
  - `chore:` — 清理、配置
  - `test:` — 测试相关
- **永远不要** force push 到 master
- **永远不要** 提交 `.o` / `.exe` / `.img` 等编译产物
- `orig/` 目录仅供参考，不对其编译

### 提交前检查清单

```bash
git status                  # 确认要提交的文件
make clean && make          # 确认编译通过
./build/fs.exe --test       # 确认 22 项全 PASS
git add src/xxx.c ...       # 只加源文件，别用 git add -A
git commit -m "fix: ..."
```

---

## 已知问题 & 待办

- [ ] `halt()` 时仅 flush 了 inode 元数据，未 flush 目录数据块（目录内容依赖 chdir 时写回的机制）
- [ ] 部分操作（creat/delete）修改内存 dir 缓冲后，不立即写回磁盘
- [ ] `aopen()` 的 append 模式实际行为是 truncate（设计意图待确认）
- [ ] 磁盘镜像路径硬编码为 `build/filesystem.img`

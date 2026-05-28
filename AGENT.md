# Operating-System Filesys Project — Agent Handoff (2026-05-25)

## Repository
- GitHub: woaiwang/Operating-System
- Default branch: master
- Collaboration: branch protection enabled (PR required, 1 approval, dismiss stale approvals)
- Current branch: `chore/import-orig`
- Current state: **compiles on Windows, zero warnings, zero errors, 22/22 tests PASS**

## Build Environment (Windows)
Use **MSYS2 UCRT64** terminal (or MinGW64).
Installed: `mingw-w64-ucrt-x86_64-toolchain` (gcc etc), make.
Build commands:
```bash
make clean && make
./build/fs.exe
```
Or without make:
```bash
gcc -std=c99 -Wall -Wextra -Iinclude src/*.c -o build/fs.exe
./build/fs.exe
```
For debug output: add `-DFS_DEBUG` to gcc flags.

## Disk Image
- Path: `build/filesystem.img`
- Size: 279,552 bytes (546 blocks × 512), fixed
- Created by `format()`, mounted by `install()`, closed by `halt()`
- Backend: `FILE *fd` with `fseek`/`fread`/`fwrite` (not memory array)

## Block Numbering Scheme
All block numbers in `s_free[]` and `di_addr[]` are **data-block indices** (0..FILEBLK-1).
Absolute disk offset = `DATASTART + data_block_index * BLOCKSIZ`.

| Absolute Block | Content |
|---|---|
| 0 | Boot block (unused) |
| 1 | Superblock (struct filsys) |
| 2..33 | Inode table (DINODEBLK=32 blocks) |
| 34..545 | Data area (FILEBLK=512 data blocks) |

DATASTART = (2 + DINODEBLK) × BLOCKSIZ = 34 × 512 = 17408

---

## Final Status: 全部完成 (ALL DONE)

### 已完成的 6 个步骤

| # | 步骤 | 结果 |
|---|------|------|
| 1 | 验证 block.c 编译 + 8 项测试 | ✅ PASS |
| 2 | format.c 改用 bfree() 喂入空闲块 | ✅ PASS |
| 3 | 移植 rdwt.c → fs_read / fs_write | ✅ PASS |
| 4 | 移植 dir.c（_dir, mkdir, chdir） | ✅ PASS |
| 5 | 移植文件操作（open, close, creat, delete, access, name） | ✅ PASS |
| 6 | 移植 log.c（login/logout）和 file.c（交互式 shell） | ✅ PASS |

---

## 源文件清单

### 原有文件（已修改）
| 文件 | 修改内容 |
|------|---------|
| `include/filesys.h` | 所有常量/结构体/原型；block 编号文档；debug_log 宏；creat 返回类型改为 int |
| `src/block.c` | balloc/bfree（V6 风格链式空闲块）；共享 chain_head 追踪；bread/bwrite |
| `src/inode.c` | iget/iput（哈希链+磁盘加载）；ialloc/ifree（超级块 inode 缓存） |
| `src/format.c` | 创建磁盘镜像；初始化超级块+inode 列表；空闲块通过 bfree() 喂入；初始化 sys_ofile/user 表 |
| `src/install.c` | 挂载已有镜像；读取超级块；初始化所有内存表 |
| `src/halt.c` | 写回超级块；关闭磁盘镜像；退出 |
| `src/main.c` | 22 项自测（8 项 A 层 + 14 项 B 层集成测试） |

### 新增文件
| 文件 | 功能 |
|------|------|
| `src/globals.c` | 所有 extern 全局变量定义 |
| `src/name.c` | namei（按名查找返回 d_ino）；iname（找空槽位存储名字） |
| `src/access.c` | access（权限检查：other → group → user，通过 inode 号调用） |
| `src/open.c` | aopen（打开文件：查找、权限检查、分配 sys_ofile + user fd） |
| `src/close.c` | close（释放 sys_ofile + user fd） |
| `src/creat.c` | creat（新建文件或截断已有文件，返回 1-based fd） |
| `src/delete.c` | delete（清除目录项，减少链接计数） |
| `src/dir.c` | _dir（列目录）；mkdir（创建子目录）；chdir（切换目录） |
| `src/rdwt.c` | fs_read / fs_write（从 read/write 改名以避开 libc 冲突） |
| `src/log.c` | login（用户名+密码认证）；logout（关闭全部文件并登出） |
| `src/file.c` | 交互式文件系统 shell（替代 orig/FILE.C 的 libc fopen 测试程序） |

### 原始参考文件（不编译）
`orig/` 目录：20 个原始教师代码文件，仅作参考，永不对其编译。

---

## FD（文件描述符）约定
- `aopen()` 返回 `unsigned short`，0 = 失败，有效 fd ≥ 1
- `creat()` 返回 `int`，-1 = 失败，有效 fd ≥ 1
- `close()` / `fs_read()` / `fs_write()` 接受 1-based fd
- 两者均跳过 `user_ofile[0]`，使 fd=0 明确表示失败

---

## 全部 22 项测试结果（ALL PASS）

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
root dir balloc:               PASS (block=509)
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

构建命令：`gcc -std=c99 -Wall -Wextra -Iinclude src/*.c -o build/fs.exe`
→ **零警告，零错误**

---

## 原始代码 Bug 修复清单（共 30 项）

### A 层（block/inode 分配）
| 原始文件 | Bug | 修复 |
|----------|-----|------|
| orig/ballfre.c | `bfree()` 从未将 block_num 推入 s_free[] — 块丢失 | 显式推入 s_free[] |
| orig/ballfre.c | `bfree()` 写链块前未 fseek | spill_to_chain() 先 fseek |
| orig/ballfre.c | `balloc()` 读链块前未 fseek | refill_from_chain() 先 fseek |
| orig/igetput.c | 空哈希链上 `newinode->i_forw->i_back` 空指针解引用 | 先检查 NULL |
| orig/igetput.c | `iput()` 调用 `balloc()` 释放块（应为 `bfree()`） | 改为 `bfree()` |
| orig/iallfre.c | `ifree()` 写入 `s_inode[NICINOD]`（越界，索引 50） | 缓存满时仅更新 rinode |
| orig/iallfre.c | `ialloc()` while 条件 `\|\|` 应为 `&&` | 修正为 `&&` |
| orig/iallfre.c | `ialloc()` 无可用 inode 时不返回 NULL | 新增 NULL 返回 |
| src/format.c | `unsigned int i` 导致无限循环（负数回绕） | 改为 `int i` |

### B 层（文件/目录操作）
| 原始文件 | Bug | 修复 |
|----------|-----|------|
| orig/name.c | `namei()` 硬编码 `dir.direct[1]` 而非 `dir.direct[i]` | 修正为 `[i]` |
| orig/name.c | `namei()` 返回目录索引而非 d_ino | 返回 d_ino |
| orig/name.c | `iname()` strcpy 方向反了（从空目录项复制到 name） | 修正方向 |
| orig/access.c | `defualt` 拼写错误；READ 分支两次检查 ODIREAD | 修正拼写；第二次改为 GDIREAD |
| orig/open.c | `if (dinodeid != NULL)` 条件反了 | 改为 `== 0` |
| orig/open.c | `user[user_id].u_ofile[j]=1` 而非 sys_ofile 索引 `i` | 改为 `= i` |
| orig/open.c | 返回 NULL（指针）于 unsigned short 函数 | 返回 0 |
| orig/creat.c | 创建时 `di_number` 设为 0（应为 1 表示目录链接） | 设为 1 |
| orig/creat.c | 文件标志仅设 FWRITE | 改为 FREAD \| FWRITE |
| orig/creat.c | user fd 循环从 0 开始（fd=0 与失败标记歧义） | 从 1 开始 |
| orig/delete.c | namei 返回 0 时空指针解引用 | 新增检查 |
| orig/delete.c | 删除后目录项未清除 | 新增条目清理 |
| orig/dir.c | `%DIRSIZs` 格式化字符串错误 | 修正为 `%14s` |
| orig/dir.c | `_dir()` 嵌套循环复用变量 `i`，破坏外层循环 | 内层改为 `k` |
| orig/dir.c | `chdir()` 将权限位（DEFAULTMODE）当作操作模式传给 access | 改为 EXICUTE |
| orig/dir.c | `chdir()` 写回循环使用未初始化的索引 `j` | 重写压缩+写回逻辑 |
| orig/rdwt.c | `fd` 形参遮盖全局 `FILE *fd` | 改名为 `fildes` |
| orig/rdwt.c | `fs_read()` 最后部分块读取后 `dst` 指针未推进 | 添加 `dst += size` |
| orig/log.c | login 中 `strcmp()` 条件反了 | 修正为 `== 0` |
| orig/log.c | `user[i].u_gid` 用了错误的索引（`i` 而非 `j`） | 修正为 `[j]` |

---

## Dev Workflow Rules
- 永远不要在 master 上搞坏构建。使用功能分支 + PR。
- 小步提交；每次提交必须能编译。
- `orig/` 目录导入后保持不动。
- 优先使用标准 C；MSYS2 UCRT64 工具链。
- 调试输出置于 `#ifdef FS_DEBUG` / `debug_log()` 后 — 默认干净输出。
- 如原始代码依赖 swindow.cpp 或 TurboC UI，用 printf 替代。
- **原始代码有 bug** — 在新代码中修复，用测试证明修复，在注释中记录。

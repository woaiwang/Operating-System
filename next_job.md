# Operating-System Filesys Project — Next Jobs

## Project Overview

这是一个 **Unix V6 风格文件系统模拟器**（操作系统课程项目），用 C99 编写。

- **仓库**: woaiwang/Operating-System
- **主分支**: master
- **当前方案**: zzy 的 V6 Unix 文件系统（clivena 的 FAT 方案已被放弃）
- **构建**: MSYS2 UCRT64 终端，`make clean && make && ./build/fs.exe`
- **测试**: 22/22 全部通过（8 个 A 层 + 14 个 B 层集成测试）

### 已完成的模块

| 层级 | 文件 | 功能 |
|------|------|------|
| A 层 | `src/block.c` | 块分配/释放/读写（V6 链式空闲块） |
| A 层 | `src/inode.c` | inode 分配/释放/获取/放回（哈希链） |
| A 层 | `src/format.c` | 创建磁盘镜像、初始化超级块 |
| A 层 | `src/install.c` | 挂载已有磁盘镜像 |
| A 层 | `src/halt.c` | 写回超级块、关闭镜像 |
| B 层 | `src/name.c` | 目录中按名查找/存储 |
| B 层 | `src/access.c` | 权限检查（user→group→other） |
| B 层 | `src/open.c` | 打开文件 |
| B 层 | `src/close.c` | 关闭文件 |
| B 层 | `src/creat.c` | 创建文件 |
| B 层 | `src/delete.c` | 删除文件 |
| B 层 | `src/dir.c` | 列目录、mkdir、chdir |
| B 层 | `src/rdwt.c` | 文件读写（fs_read/fs_write） |
| B 层 | `src/log.c` | 用户登录/注销 |
| Shell | `src/file.c` | 交互式命令行（已写好，未接入入口） |
| 入口 | `src/main.c` | 目前只跑测试套件然后退出 |
| 全局 | `src/globals.c` | 全局变量定义 |
| 头文件 | `include/filesys.h` | 所有结构体、常量、函数声明 |

### 当前问题

1. `main.c` 只跑测试，没有启动交互式 shell（`file.c` 的 `file_main()` 写好了但没人调）
2. 测试代码嵌在 main 里，没有独立的测试文件
3. 根目录有个 `filesystem` 二进制文件（279KB）不该提交
4. build 目录下的 .o 和 fs.exe 虽然 .gitignore 了但 clivena 的 PR 里提交过
5. clivena 的 `feat/filesystem-project` 分支需要清理

---

## 任务清单

每完成一个任务就提交一次（commit message 格式见末尾"提交规范"）。

---

### 任务 1: 仓库清理（Housekeeping）

**目标**: 清理不该入库的文件，确保 repo 干净。

**具体步骤**:

1. **删除根目录的 `filesystem` 二进制文件**
   ```
   rm filesystem
   ```
   这个 279KB 的文件是旧 build 残留，不应该在 git 里。

2. **确认 .gitignore 完整**
   当前 `.gitignore` 内容：
   ```
   /build/
   /*.exe
   /*.o
   /*.obj
   /*.a
   /*.dll
   /filesystem
   .vscode/
   ```
   这个已经正确了，`/build/` 会忽略整个 build 目录。

3. **检查 build 目录是否被 git 追踪**
   ```
   git ls-files build/
   ```
   如果有输出，说明 build 下的文件被追踪了，需要用 `git rm --cached` 清除。

4. **确保 build 目录存在但内容不被追踪**
   确认 `build/filesystem.img` 也不该被追踪的话，整个 `/build/` 规则已经覆盖。

5. **提交**
   ```
   git add -A
   git commit -m "chore: clean up repo, remove stale binary artifacts"
   ```

**验证标准**: `git status` 显示干净，没有 .o/.exe 文件被追踪。

---

### 任务 2: 分离测试代码 + 接入交互式 Shell

**目标**: 把测试从 main.c 抽出来，让 main 有两个模式——跑测试 或 启动交互式 shell。

**为什么**: 当前 `src/file.c` 里有一个完整的交互式 shell (`file_main()`)，但没人调用它。main.c 只跑测试然后退出。要让用户能真正"用"这个文件系统。

**具体步骤**:

#### 2a: 创建 `src/test.c`，把测试代码搬过去

1. 新建 `src/test.c`，内容包含：
   - `#include "filesys.h"`
   - `#include <stdio.h>`
   - `#include <string.h>`
   - 一个函数 `int run_all_tests(void)`，返回 0=全部通过，非0=有失败

2. 把 `src/main.c` 中第 10-284 行的测试代码搬进 `run_all_tests()` 函数。

3. 在 `include/filesys.h` 中添加声明：
   ```c
   int run_all_tests(void);
   int file_main(void);
   ```

#### 2b: 重写 `src/main.c`

新的 main.c 逻辑：
```c
#include "filesys.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        /* 测试模式 */
        return run_all_tests();
    } else {
        /* 交互模式：需要手动 format 或 install */
        printf("Use 'format' to create a new filesystem, or 'install' to mount.\n");
        printf("Use 'help' for commands.\n");
        return file_main();
    }
}
```

#### 2c: 更新 Makefile

在 Makefile 的 `OBJS` 那里，新加的 `src/test.c` 会自动被 `$(wildcard src/*.c)` 发现，不需要手动改 Makefile。

#### 2d: 编译并测试

```bash
make clean && make
```

先跑测试模式确认 22 项全过：
```bash
./build/fs.exe --test
```

再试验交互模式：
```bash
./build/fs.exe
```
进去后输入 `format`，然后 `mkdir testdir`，`dir` 看看有没有输出，`halt` 退出。

#### 2e: 提交

```
git add -A
git commit -m "refactor: extract tests to test.c, wire up interactive shell"
```

**验证标准**: 
- `./build/fs.exe --test` → 22/22 PASS
- `./build/fs.exe` → 进入交互 shell，format/dir/mkdir/halt 都能用

---

### 任务 3: 修复交互式 Shell 的已知问题

**目标**: 让交互式 shell 更好用，修掉已知缺陷。

**具体步骤**:

1. **format 后自动初始化根目录**
   
   当前 shell 的 `format` 命令（file.c:65）只是调了 `format()` 然后 `iget(1)`，但没有像 test 代码那样初始化 `.` 和 `..` 目录项。需要补充初始化逻辑。

   参考 `src/main.c` 第 110-157 行的 root dir init 代码，把这段逻辑搬到 `file.c` 的 format 命令里（或者更好的做法：写到 `format()` 函数本身或者一个新的 `init_root_dir()` 函数中）。

2. **chdir 后 dir 显示正确内容**
   
   测试 `chdir("subdir")` 后调用 `_dir()`，确认 ls 出来的是子目录的内容而非旧目录内容。如有 bug 则修。

3. **creat/open 返回 fd 的一致性**
   
   当前 `creat()` 返回 int（-1 表示失败），`aopen()` 返回 unsigned short（0 表示失败）。在 shell 里 creat 成功后 fd 应该 >= 1，但 file.c:102-105 的判断写的是 `fd >= 0`。检查一下逻辑是否正确。

4. **每改完一处，编译验证**
   ```bash
   make clean && make && ./build/fs.exe --test
   ```
   确保 22 项测试不挂。

5. **提交**（每个修复一个 commit，或者一起提交如果改动小）
   ```
   git add -A
   git commit -m "fix: shell format auto-init root dir, fix fd display"
   ```

**验证标准**: 交互模式下 format → mkdir → chdir → dir → creat → write → read → close → delete 全流程跑通。

---

### 任务 4（可选）: 目录持久化改进

**目标**: 让 chdir 切换目录时，旧目录内容正确写回磁盘。

**当前状态**: `chdir()` 在 `src/dir.c` 中已经有写回逻辑，但只在 chdir 时触发。如果用户改了当前目录（创建/删除了文件），然后直接 halt，改动可能丢失。

**具体步骤**:

1. 在 `halt()` 中增加一步：把当前 `dir` 写回 `cur_path_inode`。
2. 或者：每次 `creat`/`delete`/`mkdir` 后立即写回（简单但慢）。

建议方案 1（halt 时写回），改动最小。

3. 提交：
   ```
   git commit -m "fix: flush current directory to disk on halt"
   ```

---

### 任务 5（可选）: 清理 clivena 的分支

**目标**: 整理远程分支，删除已关闭的 PR 分支。

**具体步骤**:

```bash
# 删除远程的 feat/filesystem-project 分支（PR 已关闭）
git push origin --delete feat/filesystem-project

# 删除远程的 docs/readme 分支（已合并进 master）
git push origin --delete docs/readme
```

提交不需要，这是远程操作。

---

## 架构速查

### 磁盘布局
```
绝对块 0         → 引导块（未使用）
绝对块 1         → 超级块 (struct filsys)
绝对块 2..33     → inode 表 (DINODEBLK=32 块, 每块 16 个 dinode, 共 512 个)
绝对块 34..545   → 数据区 (FILEBLK=512 块, 每块 512 字节)
```

- `DATASTART = 17408`（字节偏移，指向第一个数据块）
- `s_free[]` 和 `di_addr[]` 里存的是**数据块索引**（0..511），不是绝对块号
- `bread()`/`bwrite()` 吃的是**绝对块号**

### 关键文件
| 文件 | 作用 |
|------|------|
| `include/filesys.h` | 所有常量和结构体定义（改这里影响全局） |
| `src/globals.c` | 全局变量定义 |
| `src/main.c` | 入口点 |
| `src/test.c` | 测试（任务 2 新建） |
| `src/file.c` | 交互式 shell |
| `Makefile` | 构建配置 |

### FD 约定
- `aopen()` 返回 `unsigned short`，0=失败，fd≥1
- `creat()` 返回 `int`，-1=失败，fd≥1
- `close()`/`fs_read()`/`fs_write()` 吃 1-based fd
- `user[].u_ofile[0]` 故意留空，让 fd=0 明确表示失败

### 调试
编译时加 `-DFS_DEBUG` 开启调试输出：
```bash
gcc -std=c99 -Wall -Wextra -DFS_DEBUG -Iinclude src/*.c -o build/fs.exe
```

---

## 提交规范

- **每次只做一件事**，做完就 commit
- **每个 commit 必须能编译**（不能编译的代码不提交）
- **commit message 格式**: `<type>: <简短描述>`
  - `chore:` — 清理、配置、杂务
  - `feat:` — 新功能
  - `fix:` — 修 bug
  - `refactor:` — 重构（不改变功能）
  - `test:` — 测试相关
- **永远不要 force push 到 master**
- **永远不要 `git add -A` 前不检查** — 先用 `git status` 确认要提交什么

### 工作流程
```
1. git status                  # 看当前状态
2. git add src/xxx.c ...       # 只加你要提交的文件，别用 git add .
3. git commit -m "fix: ..."    # 提交
4. git log --oneline -3        # 确认提交成功
```

**绝对不要**:
- 提交 .o / .exe / .img 等编译产物
- 提交时跳过 hook（--no-verify）
- 在 master 上做实验性改动（应该开新分支）

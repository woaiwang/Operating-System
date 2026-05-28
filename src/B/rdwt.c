/**
 * =========================================================================
 * rdwt.c — 文件读写模块（任务B：文件操作层）
 * =========================================================================
 *
 * 本模块实现文件数据的读取和写入，是 B 层最复杂的模块。
 * 涉及跨块读写、非对齐偏移处理、按需块分配等逻辑。
 *
 * ┌─────────────────────────────────────────────────────────────────────┐
 * │                    任务A（底层）依赖关系                              │
 * ├─────────────────────────────────────────────────────────────────────┤
 * │ 依赖项            │ 定义位置          │ 说明                          │
 * ├────────────────────┼───────────────────┼──────────────────────────────┤
 * │ balloc()           │ src/block.c:88    │ 任务A：从空闲块栈(s_free[])  │
 * │                    │                   │ 分配一个数据块，返回块索引    │
 * │                    │                   │ 栈空时从磁盘链块重新填充      │
 * │                    │                   │ 磁盘满返回 DISKFULL=65535     │
 * ├────────────────────┼───────────────────┼──────────────────────────────┤
 * │ fd (全局FILE*)     │ src/globals.c:8   │ 任务A：磁盘镜像文件的文件指针 │
 * │                    │                   │ 由 format()/install() 打开    │
 * │                    │                   │ 所有磁盘I/O通过fseek+fread/   │
 * │                    │                   │ fwrite 操作此文件              │
 * ├────────────────────┼───────────────────┼──────────────────────────────┤
 * │ sys_ofile[]        │ src/globals.c:14  │ 任务A：系统打开文件表         │
 * │                    │                   │ f_off: 当前读写偏移量(字节)   │
 * │                    │                   │ f_inode: 指向文件inode        │
 * │                    │                   │ f_flag: 读写模式标志          │
 * ├────────────────────┼───────────────────┼──────────────────────────────┤
 * │ user[]             │ src/globals.c:20  │ 任务A：用户表                 │
 * │                    │                   │ u_ofile[fd] → sys_ofile 索引  │
 * ├────────────────────┼───────────────────┼──────────────────────────────┤
 * │ user_id            │ src/globals.c:26  │ 任务A：当前用户索引           │
 * ├────────────────────┼───────────────────┼──────────────────────────────┤
 * │ BLOCKSIZ 常量      │ include/filesys.h │ 值=512，每块字节数            │
 * ├────────────────────┼───────────────────┼──────────────────────────────┤
 * │ DATASTART 常量     │ include/filesys.h │ 值=(2+32)*512=17408           │
 * │                    │                   │ 数据区起始字节偏移            │
 * │                    │                   │ di_addr[i] 存的是数据块索引   │
 * │                    │                   │ 磁盘位置 = DATASTART +        │
 * │                    │                   │           di_addr[i]*BLOCKSIZ │
 * ├────────────────────┼───────────────────┼──────────────────────────────┤
 * │ NADDR 常量         │ include/filesys.h │ 值=10，直接索引块数上限       │
 * ├────────────────────┼───────────────────┼──────────────────────────────┤
 * │ NOFILE/SYSOPENFILE │ include/filesys.h │ fd 合法性检查                 │
 * ├────────────────────┼───────────────────┼──────────────────────────────┤
 * │ DISKFULL 常量      │ include/filesys.h │ 值=65535，balloc 失败标志     │
 * └────────────────────┴───────────────────┴──────────────────────────────┘
 *
 * 磁盘寻址说明（重要！）：
 *   inode->di_addr[k] 存储的是"数据块索引"（0~FILEBLK-1），
 *   而非绝对块号。转换为磁盘字节偏移的公式：
 *     磁盘偏移 = DATASTART + di_addr[k] * BLOCKSIZ
 *   其中 DATASTART = (2 + DINODEBLK) * BLOCKSIZ = 17408
 *
 *   例如：di_addr[0] = 5 表示数据区的第5块，
 *         磁盘位置 = 17408 + 5 * 512 = 19968 字节处
 *
 * 命名说明：
 *   - 函数名从 read/write 改为 fs_read/fs_write，避免与 libc 的 POSIX 函数冲突
 *   - 参数名从 fd 改为 fildes，避免与全局 FILE *fd（磁盘镜像文件指针）冲突
 *     （原始 bug：本地 int fd 遮蔽了全局 FILE *fd，导致磁盘 I/O 操作错误的文件）
 */

#include "filesys.h"   /* 所有结构体、常量、全局变量声明 */
#include <stdio.h>     /* printf(), fseek(), fread(), fwrite() */

/**
 * fs_read — 从文件中读取数据
 * @fildes: 用户文件描述符（1~NOFILE-1）
 * @buf:    输出缓冲区（调用者分配）
 * @size:   期望读取的字节数
 * @return: 实际读取的字节数，失败返回 0
 *
 * 读取流程：
 *   1) 通过两级映射 fildes → sys_ofile → inode
 *   2) 根据当前偏移量 f_off 计算起始块和块内偏移
 *   3) 分三段处理：首个非完整块 → 连续完整块 → 末尾非完整块
 *   4) 每段都检查 di_addr[] 是否有效（非0），防止读取未分配区域
 *   5) 更新 f_off（偏移量前进）
 */
unsigned int fs_read(int fildes, char *buf, unsigned int size) {
    unsigned long off;                       /* 当前读写偏移量（从 f_off 复制） */
    unsigned int block_off;                  /* 块内偏移（0~BLOCKSIZ-1） */
    int block_idx;                           /* 当前数据块在 di_addr[] 中的索引 */
    int i, nblocks;                          /* i: 循环变量, nblocks: 完整块数量 */
    struct inode *inode;                     /* 文件 inode 指针 */
    unsigned int sys_no;                     /* 系统打开文件表索引 */
    char *dst;                               /* 目标写入指针（在 buf 中移动） */

    /* ── 参数合法性检查 ── */
    if (fildes < 0 || fildes >= NOFILE) return 0;  /* fd 超出范围 */

    /* ── 两级映射：用户fd → 系统打开文件表 ── */
    sys_no = user[user_id].u_ofile[fildes];  /* 查用户表获取 sys_ofile 索引 */
    if (sys_no == SYSOPENFILE + 1 || sys_no >= SYSOPENFILE) return 0;
    /*     ^^^^^^^^^^^^^^^^^^^^^^      ^^^^^^^^^^^^^^^^^^^^^
     *     fd未打开(SYSOPENFILE+1=41)   索引越界（无效） */

    /* ── 检查文件是否以读模式打开 ── */
    if (!(sys_ofile[sys_no].f_flag & FREAD)) { /* f_flag 位与 FREAD(=00001) */
        printf("\n文件未以读模式打开\n");
        return 0;
    }

    /* ── 获取 inode ── */
    inode = sys_ofile[sys_no].f_inode;       /* 系统打开文件表项关联的 inode */
    if (!inode) return 0;                    /* 防御性检查 */

    /* ── 计算实际可读字节数 ── */
    off = sys_ofile[sys_no].f_off;           /* 获取当前文件偏移量 */
    if (off + size > inode->di_size)         /* 不能读取超过文件大小的数据 */
        size = (unsigned int)(inode->di_size - off);  /* 截断到文件末尾 */
    if (size == 0) return 0;                 /* 已在文件末尾或超出，无可读数据 */

    /* ── 计算起始位置 ── */
    dst = buf;                               /* 初始化目标指针 */
    block_off = (unsigned int)(off % BLOCKSIZ); /* 块内偏移 = 偏移量 % 512 */
    block_idx = (int)(off / BLOCKSIZ);        /* 块索引   = 偏移量 / 512 */
                                              /* 例如 off=600: block_idx=1, block_off=88 */

    /* ═══════════════════════════════════════════════════════════════════
     * 阶段一：读取第一个非完整块（如果 block_off > 0）
     * ═══════════════════════════════════════════════════════════════════ */
    if (block_off > 0) {                     /* 偏移量不在块边界上 */
        unsigned int chunk = BLOCKSIZ - block_off;  /* 当前块剩余字节数 */
        if (chunk > size) chunk = size;      /* 不超过请求的字节数 */

        /* 安全检查：该数据块必须已分配（di_addr[] 非 0） */
        if (inode->di_addr[block_idx] == 0)  /* 块未分配，无法读取 */
            return (unsigned int)(dst - buf); /* 返回已读取的字节数（此时为0） */

        /* 磁盘定位：DATASTART + 数据块索引 * BLOCKSIZ + 块内偏移 */
        /* DATASTART=17408，di_addr[i]是数据块索引(0~511)，乘以512得字节偏移 */
        fseek(fd, DATASTART + (long)inode->di_addr[block_idx] * BLOCKSIZ + block_off, SEEK_SET);
        /*         ^^^^^^^^   ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^   ^^^^^^^^^^
         *         数据区起点  该数据块的字节偏移                  块内偏移 */
        fread(dst, 1, chunk, fd);            /* 从磁盘读取 chunk 字节到 dst */
        dst += chunk;                        /* 目标指针前进 */
        size -= chunk;                       /* 剩余待读字节数减少 */
        block_idx++;                         /* 移动到下一个数据块 */
    }

    /* ═══════════════════════════════════════════════════════════════════
     * 阶段二：读取中间的完整块（每次读 BLOCKSIZ=512 字节）
     * ═══════════════════════════════════════════════════════════════════ */
    nblocks = (int)(size / BLOCKSIZ);        /* 完整块的数量 */
    for (i = 0; i < nblocks; i++) {          /* 逐块读取 */
        /* 安全检查：当前数据块必须已分配 */
        if (inode->di_addr[block_idx + i] == 0) break;  /* 遇到未分配块，停止 */
        /* 磁盘定位到该数据块的开头 */
        fseek(fd, DATASTART + (long)inode->di_addr[block_idx + i] * BLOCKSIZ, SEEK_SET);
        fread(dst, 1, BLOCKSIZ, fd);         /* 读取整块 512 字节 */
        dst += BLOCKSIZ;                     /* 目标指针前进 512 字节 */
        size -= BLOCKSIZ;                    /* 剩余字节数减少 512 */
    }

    /* ═══════════════════════════════════════════════════════════════════
     * 阶段三：读取最后一个非完整块（如果还有剩余字节）
     * ═══════════════════════════════════════════════════════════════════ */
    if (size > 0) {                          /* 还有不满一块的数据要读 */
        /* 安全检查：目标块必须已分配 */
        if (inode->di_addr[block_idx + nblocks] == 0)
            return (unsigned int)(dst - buf); /* 返回已读字节数 */
        fseek(fd, DATASTART + (long)inode->di_addr[block_idx + nblocks] * BLOCKSIZ, SEEK_SET);
        fread(dst, 1, size, fd);             /* 读取剩余的 size 字节 */
        dst += size;                         /* 目标指针前进 */
    }

    /* ── 更新文件偏移量 ── */
    sys_ofile[sys_no].f_off += (unsigned long)(dst - buf);
    /*                          ^^^^^^^^^^^^^^^^^^^^^^^^^
     *                          dst - buf = 实际读取的总字节数 */

    return (unsigned int)(dst - buf);        /* 返回实际读取的字节数 */
}

/**
 * fs_write — 向文件中写入数据
 * @fildes: 用户文件描述符（1~NOFILE-1）
 * @buf:    输入缓冲区（要写入的数据）
 * @size:   要写入的字节数
 * @return: 实际写入的字节数，失败返回 0
 *
 * 写入流程：
 *   1) 通过两级映射获取 inode
 *   2) 确保起始块已分配（未分配则调用 balloc）
 *   3) 分三段处理：首个非完整块 → 连续完整块 → 末尾非完整块
 *   4) 每段在写之前检查并分配所需的数据块
 *   5) 更新 f_off 和 di_size
 *
 * 与 fs_read 的关键区别：写入时需要按需分配新块（balloc），
 * 而读取时仅检查块是否已分配。
 */
unsigned int fs_write(int fildes, const char *buf, unsigned int size) {
    unsigned long off;                       /* 当前读写偏移量 */
    unsigned int block_off;                  /* 块内偏移 */
    int block_idx;                           /* di_addr[] 中的块索引 */
    int i, nblocks;                          /* i:循环变量, nblocks:完整块数 */
    struct inode *inode;                     /* 文件 inode 指针 */
    unsigned int sys_no;                     /* 系统打开文件表索引 */
    unsigned int blk;                        /* balloc() 返回的新块号 */
    const char *src;                         /* 源数据指针（在 buf 中移动） */
    unsigned int total_written;              /* 累计写入字节数 */

    /* ── 参数合法性检查 ── */
    if (fildes < 0 || fildes >= NOFILE) return 0;

    /* ── 两级映射 ── */
    sys_no = user[user_id].u_ofile[fildes];
    if (sys_no == SYSOPENFILE + 1 || sys_no >= SYSOPENFILE) return 0;

    /* ── 写权限检查 ── */
    if (!(sys_ofile[sys_no].f_flag & FWRITE)) { /* f_flag 位与 FWRITE(=00002) */
        printf("\n文件未以写模式打开\n");
        return 0;
    }

    /* ── 获取 inode ── */
    inode = sys_ofile[sys_no].f_inode;
    if (!inode) return 0;

    /* ── 初始化写入状态 ── */
    off  = sys_ofile[sys_no].f_off;          /* 当前偏移量 */
    src  = buf;                              /* 源数据指针 */
    total_written = 0;                       /* 累计写入字节数 */
    block_off = (unsigned int)(off % BLOCKSIZ); /* 块内偏移 */
    block_idx = (int)(off / BLOCKSIZ);        /* 块索引 */

    /* ── 确保起始块已分配 ── */
    if (block_idx >= NADDR) {                /* NADDR=10，超出直接索引范围 */
        printf("\n文件过大\n");              /* 本系统不支持间接索引，最多10个直接块 */
        return 0;                            /* 10块 × 512字节 = 最大文件 5120 字节 */
    }
    if (inode->di_addr[block_idx] == 0) {    /* 该块尚未分配 */
        blk = balloc();                      /* [任务A] 分配一个新的数据块 */
                                             /* balloc 从 filsys.s_free[]栈顶弹出块号 */
                                             /* 栈空时从磁盘空闲链块重新填充 */
        if (blk == DISKFULL) return total_written;  /* 磁盘满，返回已写入的字节数 */
        inode->di_addr[block_idx] = blk;     /* 将新块号记录到 inode 的地址数组中 */
    }

    /* ═══════════════════════════════════════════════════════════════════
     * 阶段一：写入第一个非完整块
     * ═══════════════════════════════════════════════════════════════════ */
    if (block_off > 0) {                     /* 偏移量不在块边界，先填充当前块的剩余部分 */
        unsigned int chunk = BLOCKSIZ - block_off;  /* 当前块剩余空间 */
        if (chunk > size) chunk = size;      /* 不超过待写入的字节数 */

        /* 磁盘定位：与 fs_read 相同的寻址公式 */
        fseek(fd, DATASTART + (long)inode->di_addr[block_idx] * BLOCKSIZ + block_off, SEEK_SET);
        fwrite(src, 1, chunk, fd);           /* 写入 chunk 字节到磁盘 */
        src += chunk;                        /* 源指针前进 */
        size -= chunk;                       /* 剩余待写字节数减少 */
        total_written += chunk;              /* 累计写入数增加 */
        block_idx++;                         /* 移动到下一个数据块 */
    }

    /* ═══════════════════════════════════════════════════════════════════
     * 阶段二：写入中间的完整块（每块 BLOCKSIZ=512 字节）
     * ═══════════════════════════════════════════════════════════════════ */
    nblocks = (int)(size / BLOCKSIZ);        /* 计算完整块数量 */
    for (i = 0; i < nblocks; i++) {          /* 逐块写入 */
        if (block_idx + i >= NADDR) break;   /* 超出直接索引范围，停止 */

        /* 如果该块尚未分配，分配新块 */
        if (inode->di_addr[block_idx + i] == 0) {
            blk = balloc();                  /* [任务A] 分配数据块 */
            if (blk == DISKFULL) {           /* 磁盘空间不足 */
                sys_ofile[sys_no].f_off += total_written;  /* 更新偏移量 */
                return total_written;        /* 返回已写入字节数（尽力而为语义） */
            }
            inode->di_addr[block_idx + i] = blk;  /* 记录新块号 */
        }

        /* 写入完整块 */
        fseek(fd, DATASTART + (long)inode->di_addr[block_idx + i] * BLOCKSIZ, SEEK_SET);
        fwrite(src, 1, BLOCKSIZ, fd);        /* 写入 512 字节 */
        src += BLOCKSIZ;
        size -= BLOCKSIZ;
        total_written += BLOCKSIZ;
    }

    /* ═══════════════════════════════════════════════════════════════════
     * 阶段三：写入最后一个非完整块
     * ═══════════════════════════════════════════════════════════════════ */
    if (size > 0) {                          /* 还有不满一块的数据要写 */
        int final_idx = block_idx + nblocks; /* 目标块在 di_addr[] 中的索引 */
        if (final_idx >= NADDR) {            /* 超出范围 */
            sys_ofile[sys_no].f_off += total_written;
            return total_written;
        }

        /* 分配新块（如果需要） */
        if (inode->di_addr[final_idx] == 0) {
            blk = balloc();                  /* [任务A] 分配数据块 */
            if (blk == DISKFULL) {
                sys_ofile[sys_no].f_off += total_written;
                return total_written;
            }
            inode->di_addr[final_idx] = blk;
        }

        /* 写入剩余数据 */
        fseek(fd, DATASTART + (long)inode->di_addr[final_idx] * BLOCKSIZ, SEEK_SET);
        fwrite(src, 1, size, fd);            /* 写入最后 size 字节 */
        total_written += size;               /* 累计 */
    }

    /* ── 更新文件状态 ── */
    sys_ofile[sys_no].f_off += total_written; /* 偏移量前进 */
    if (sys_ofile[sys_no].f_off > inode->di_size)  /* 如果写到了文件末尾之后 */
        inode->di_size = (unsigned short)sys_ofile[sys_no].f_off;  /* 更新文件大小 */
        /*               ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
         *               注意：di_size 为 unsigned short（最大65535），
         *               对于超过65535字节的文件会截断。
         *               但受 NADDR=10 限制，最大文件为 10×512=5120 字节 */

    return total_written;                    /* 返回实际写入的字节数 */
}

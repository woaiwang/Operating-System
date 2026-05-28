/**
 * =========================================================================
 * name.c — 路径名解析模块（任务B：文件操作层）
 * =========================================================================
 *
 * 本模块提供文件名到 inode 号的映射，是文件操作层最基础的模块。
 * 几乎所有 B 层操作（creat/open/delete/mkdir/chdir）都依赖本模块。
 *
 * ┌─────────────────────────────────────────────────────────────────────┐
 * │                    任务A（底层）依赖关系                              │
 * ├─────────────────────────────────────────────────────────────────────┤
 * │ 依赖项            │ 定义位置          │ 说明                          │
 * ├────────────────────┼───────────────────┼──────────────────────────────┤
 * │ dir (全局变量)     │ src/globals.c:23  │ 当前工作目录的内存表示        │
 * │                    │                   │ struct dir { direct[128];     │
 * │                    │                   │             int size; }       │
 * │                    │                   │ 由 format.c/install.c 初始化  │
 * ├────────────────────┼───────────────────┼──────────────────────────────┤
 * │ DIEMPTY 常量       │ include/filesys.h │ 值=00000(八进制)，表示空inode │
 * │                    │                   │ 用于判断目录项是否有效        │
 * ├────────────────────┼───────────────────┼──────────────────────────────┤
 * │ DIRNUM 常量        │ include/filesys.h │ 值=128，目录最大容量           │
 * ├────────────────────┼───────────────────┼──────────────────────────────┤
 * │ DIRSIZ 常量        │ include/filesys.h │ 值=14，文件名最大长度          │
 * ├────────────────────┼───────────────────┼──────────────────────────────┤
 * │ struct direct      │ include/filesys.h │ 目录项：d_name[14] + d_ino    │
 * └────────────────────┴───────────────────┴──────────────────────────────┘
 *
 * 本模块提供两个核心函数：
 *   namei() — 按名查找，返回 inode 号（调用者传给 iget()）
 *   iname() — 找空槽位并存入文件名，返回目录索引
 */

#include "filesys.h"   /* 所有结构体、常量、全局变量声明 */
#include <string.h>    /* strcmp(), strcpy() */
#include <stdio.h>     /* printf() */

/**
 * namei — 在当前目录中按文件名查找
 * @name: 要查找的文件名（不含路径，仅当前目录）
 * @return: 匹配目录项的 d_ino（磁盘 inode 号），未找到返回 0
 *
 * 调用链：namei() → 遍历 dir.direct[] → 返回 d_ino → 调用者用 iget(d_ino)
 *         获取内存 inode（iget() 是任务A提供的函数，见 src/inode.c）
 *
 * 注意：返回值是 inode 号（unsigned int），不是目录索引。
 *       如果返回 0，调用者应视为"文件不存在"。
 */
unsigned int namei(const char *name) {
    int i;  /* 目录项遍历索引 */

    /* 遍历当前目录的所有有效目录项 */
    for (i = 0; i < dir.size; i++) {         /* dir.size: 当前目录中的有效目录项数 */
                                              /* dir 是任务A在 globals.c 中定义的全局变量 */
                                              /* 由 format.c:init_root_dir() 或 chdir() 填充内容 */

        /* 条件1: d_ino != DIEMPTY — 该目录项指向有效 inode（非空槽位） */
        /*          DIEMPTY=00000(八进制)，定义在 include/filesys.h */
        /* 条件2: strcmp 返回 0 — 目录项名称与查找名称完全匹配 */
        if (dir.direct[i].d_ino != DIEMPTY &&                 /* 排除已删除/空目录项 */
            strcmp(dir.direct[i].d_name, name) == 0)          /* 按名称精确匹配 */
            return dir.direct[i].d_ino;                        /* 返回 inode 号，调用者用 iget() 获取 */

        /* 原始 bug：这里原来是 dir.direct[1]，硬编码索引 1，只检查第二个目录项 */
        /* 修复为 dir.direct[i]，正确遍历所有目录项 */
    }

    return 0;  /* 未找到匹配项。0 不是有效的 inode 号（inode 从 1 开始编号） */
}

/**
 * iname — 在目录中找空槽位并存入文件名
 * @name: 要存入的文件名
 * @return: 目录索引（0~DIRNUM-1），目录满则返回 0
 *
 * 调用链：iname() → 找 d_ino==0 的空目录项 → strcpy 写入文件名 → 返回索引
 *         调用者后续设置 dir.direct[返回值].d_ino = 新分配的 inode 号
 *
 * 注意：返回 0 可能表示索引 0 可用，也可能表示目录满。
 *       调用者应结合 dir.size >= DIRNUM 判断。
 */
unsigned short iname(const char *name) {
    int i;  /* 目录项遍历索引 */

    /* 遍历所有可能的目录槽位（0 ~ DIRNUM-1，共128个） */
    for (i = 0; i < DIRNUM; i++) {           /* DIRNUM 定义在 include/filesys.h，值=128 */

        /* d_ino == 0 表示该目录项为空（未被使用或已删除） */
        if (dir.direct[i].d_ino == 0) {      /* 找到空闲目录项 */

            /* 将文件名拷贝到目录项的 d_name 字段（最大 DIRSIZ=14 字节） */
            strcpy(dir.direct[i].d_name, name);  /* 原始 bug：strcpy 方向反了， */
                                                  /* 原代码是 strcpy(name, dir.direct[i].d_name) */
                                                  /* 会把空目录项的内容拷贝到 name 参数，导致数据损坏 */

            return (unsigned short)i;        /* 返回目录索引，转为 unsigned short 与调用处一致 */
        }
    }

    /* 遍历完所有槽位都未找到空位，目录已满 */
    printf("\n当前目录已满！\n");            /* DIRNUM=128，一般情况下很难满 */
    return 0;                                /* 返回 0 表示失败（索引 0 本身也可能是空位 */
                                             /* 但调用者通过 dir.size >= DIRNUM 即可区分） */
}

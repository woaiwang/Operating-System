/**
 * =========================================================================
 * close.c — 文件关闭模块（任务B：文件操作层）
 * =========================================================================
 *
 * 本模块实现文件描述符的关闭操作，释放用户fd槽位并递减系统打开文件引用计数。
 *
 * ┌─────────────────────────────────────────────────────────────────────┐
 * │                    任务A（底层）依赖关系                              │
 * ├─────────────────────────────────────────────────────────────────────┤
 * │ 依赖项            │ 定义位置          │ 说明                          │
 * ├────────────────────┼───────────────────┼──────────────────────────────┤
 * │ iput(inode)        │ src/inode.c:99    │ 任务A：释放inode引用。        │
 * │                    │                   │ i_count>1时仅递减计数；       │
 * │                    │                   │ i_count==1时：di_number!=0则  │
 * │                    │                   │ 写回磁盘；di_number==0则释放  │
 * │                    │                   │ 所有数据块(bfree)+归还inode   │
 * ├────────────────────┼───────────────────┼──────────────────────────────┤
 * │ sys_ofile[]        │ src/globals.c:14  │ 任务A：系统打开文件表         │
 * │                    │                   │ sys_ofile[n].f_count: 引用计数│
 * │                    │                   │ sys_ofile[n].f_inode:→inode   │
 * ├────────────────────┼───────────────────┼──────────────────────────────┤
 * │ user[]             │ src/globals.c:20  │ 任务A：用户表                 │
 * │                    │                   │ user[id].u_ofile[fd]→sys索引  │
 * ├────────────────────┼───────────────────┼──────────────────────────────┤
 * │ user_id            │ src/globals.c:26  │ 任务A：当前用户索引           │
 * ├────────────────────┼───────────────────┼──────────────────────────────┤
 * │ NOFILE 常量        │ include/filesys.h │ 值=20，每用户最大fd数         │
 * ├────────────────────┼───────────────────┼──────────────────────────────┤
 * │ SYSOPENFILE 常量   │ include/filesys.h │ 值=40，系统打开文件表大小     │
 * └────────────────────┴───────────────────┴──────────────────────────────┘
 *
 * 关闭流程：
 *   close(fd) → 查 user[user_id].u_ofile[fd] 获取 sys_no
 *            → iput(sys_ofile[sys_no].f_inode) 释放 inode 引用
 *            → sys_ofile[sys_no].f_count-- 递减引用计数
 *            → user[user_id].u_ofile[fd] = SYSOPENFILE+1 标记空闲
 */

#include "filesys.h"   /* 所有结构体、常量、全局变量声明 */

/**
 * close — 关闭一个用户文件描述符
 * @fd: 要关闭的用户文件描述符（0~NOFILE-1）
 *
 * 注意：close 不直接释放数据块。数据块的释放由 iput() 在 inode
 * 引用计数归零且 di_number==0（文件已删除）时自动完成。
 * 这也意味着 close 一个正常文件不会丢失数据 — 数据已通过 iput() 写回磁盘。
 */
void close(int fd) {
    unsigned short sys_no;                   /* 系统打开文件表索引 */
    struct inode *inode;                     /* 文件 inode 指针 */

    /* ── 参数合法性检查 ── */
    if (fd < 0 || fd >= NOFILE) return;      /* fd 超出有效范围 [0, NOFILE-1] */
                                             /* NOFILE=20，fd 必须满足 0 <= fd < 20 */

    /* ── 获取系统打开文件表索引 ── */
    /* 通过两级映射：fd → user[user_id].u_ofile[fd] → sys_ofile索引 */
    sys_no = user[user_id].u_ofile[fd];      /* user_id: 当前登录用户索引（任务A全局变量） */
                                             /* u_ofile[fd] 存储的是 sys_ofile[] 的索引 */

    /* 检查该 fd 是否有效：SYSOPENFILE+1(=41) 表示"未使用"，>=SYSOPENFILE 表示无效 */
    if (sys_no == SYSOPENFILE + 1 || sys_no >= SYSOPENFILE) return;
    /*     ^^^^^^^^^^^^^^^^^^^^^^^^    ^^^^^^^^^^^^^^^^^^^^^^^^
     *     该fd槽位空闲（未打开）      索引超出系统打开文件表范围 */

    /* ── 释放 inode 引用 ── */
    inode = sys_ofile[sys_no].f_inode;       /* 获取系统打开文件表项关联的 inode */
    iput(inode);                             /* [任务A] 释放 inode 引用 */
                                             /* iput 递减 i_count；若计数归零： */
                                             /*   - di_number != 0 → 写回磁盘 dinode */
                                             /*   - di_number == 0 → 释放所有数据块 + 归还 inode */

    /* ── 递减系统打开文件表引用计数 ── */
    /* 原始 bug：原代码无条件 f_count--，可能导致 f_count 变成负数 */
    /* （例如重复 close 同一个 fd）。现在仅当 f_count > 0 时才递减。 */
    if (sys_ofile[sys_no].f_count > 0)       /* 防止下溢 */
        sys_ofile[sys_no].f_count--;         /* 引用计数 -1 */
                                             /* 当 f_count==0 时，该槽位可被重新分配 */

    /* ── 标记用户 fd 槽位为空闲 ── */
    user[user_id].u_ofile[fd] = SYSOPENFILE + 1;  /* SYSOPENFILE+1=41，标记为"未使用" */
                                                   /* 下次 creat/open 可以重用此 fd */
}

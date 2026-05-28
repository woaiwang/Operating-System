#include "filesys.h"
#include <stdio.h>

/*
 * aopen: 为当前用户打开一个文件
 * 返回用户文件描述符索引，失败返回 0
 *
 * 原始 bug 修复：
 *  - 反转的条件："if (dinodeid != NULL)" 应检查
 *    "if (dinodeid == 0)"（文件不存在）
 *  - u_ofile[j] 错误地设为 1 而非系统打开文件表索引 i
 *  - 失败时返回 0（而非 NULL），与 unsigned short 类型一致
 *  - namei() 现在返回 d_ino 而非目录索引
 *  - 修复 FAPPEND 模式：原来错误地截断了文件（释放所有数据块），
 *    现在 FAPPEND 仅将偏移量定位到文件末尾，保留已有数据
 */
unsigned short aopen(const char *name, unsigned int mode) {
    unsigned int dinodeid;
    struct inode *inode;
    int i, j;

    dinodeid = namei(name);
    if (dinodeid == 0) {
        printf("\n文件不存在！\n");
        return 0;
    }

    inode = iget(dinodeid);
    if (!inode) return 0;

    if (!access(inode->i_ino, mode)) {
        printf("\n文件打开权限不足！\n");
        iput(inode);
        return 0;
    }

    /* 分配系统打开文件表槽位 */
    for (i = 1; i < SYSOPENFILE; i++)
        if (sys_ofile[i].f_count == 0) break;
    if (i == SYSOPENFILE) {
        printf("\n系统打开文件表已满\n");
        iput(inode);
        return 0;
    }
    sys_ofile[i].f_inode = inode;
    sys_ofile[i].f_flag  = (char)mode;
    sys_ofile[i].f_count = 1;
    /* 追加模式：偏移量定位到文件末尾；普通模式：从头开始 */
    sys_ofile[i].f_off   = (mode & FAPPEND) ? inode->di_size : 0;

    /* 分配用户文件描述符（从 1 开始，fd=0 表示失败） */
    for (j = 1; j < NOFILE; j++)
        if (user[user_id].u_ofile[j] == SYSOPENFILE + 1) break;
    if (j == NOFILE) {
        printf("\n用户打开文件表已满！\n");
        sys_ofile[i].f_count = 0;
        iput(inode);
        return 0;
    }
    user[user_id].u_ofile[j] = (unsigned short)i;

    /* FAPPEND 模式：偏移量已在上面设置为文件末尾，保留已有数据不截断 */

    return (unsigned short)j;
}

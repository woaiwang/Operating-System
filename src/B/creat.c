#include "filesys.h"
#include <stdio.h>

/*
 * creat: 为当前用户在当前目录中创建新文件（或截断已存在的文件）
 * 成功返回用户文件描述符，失败返回 -1
 *
 * 原始 bug 修复：
 *  - 修复"文件已存在"分支中未初始化的 inode 指针
 *  - namei() 现在返回 d_ino（inode 号）而非目录索引
 *  - 修复多处未初始化的结构体字段（f_count 等）
 *  - iname() 现在正确地将文件名拷贝到目录项中
 *  - 修复截断时 bfree 对零块号的无效释放
 */
int creat(const char *name) {
    unsigned int di_ino;
    struct inode *inode;
    int i, j;

    di_ino = namei(name);

    if (di_ino != 0) {
        /* 文件已存在 — 截断它 */
        inode = iget(di_ino);
        if (!inode) return -1;

        if (!access(inode->i_ino, WRITE)) {
            printf("\ncreat: 访问被拒绝\n");
            iput(inode);
            return -1;
        }

        /* 释放旧文件的所有数据块（跳过未分配块，即 di_addr[i] == 0） */
        for (i = 0; i < NADDR; i++) {
            if (inode->di_addr[i] != 0) {
                bfree(inode->di_addr[i]);
                inode->di_addr[i] = 0;
            }
        }

        inode->di_size = 0;

        /* 重置所有打开该文件实例的偏移量 */
        for (i = 0; i < SYSOPENFILE; i++)
            if (sys_ofile[i].f_inode == inode)
                sys_ofile[i].f_off = 0;

        /* 查找/复用用户文件描述符（从 1 开始，0 表示失败） */
        for (i = 1; i < NOFILE; i++) {
            if (user[user_id].u_ofile[i] == SYSOPENFILE + 1) {
                /* 查找空闲的 sys_ofile 槽位 */
                for (j = 0; j < SYSOPENFILE; j++)
                    if (sys_ofile[j].f_count == 0) break;
                if (j == SYSOPENFILE) {
                    iput(inode);
                    return -1;
                }
                user[user_id].u_ofile[i] = (unsigned short)j;
                sys_ofile[j].f_flag  = FWRITE;
                sys_ofile[j].f_count = 1;
                sys_ofile[j].f_off   = 0;
                sys_ofile[j].f_inode = inode;
                user[user_id].u_uid  = inode->di_uid;
                user[user_id].u_gid  = inode->di_gid;
                return i;
            }
        }
        iput(inode);
        return -1;
    }

    /* 新建文件 */
    inode = ialloc();
    if (!inode) return -1;

    j = (int)iname(name);  /* 查找空槽位并存入文件名 */
    if (j == 0 && dir.size >= DIRNUM) {
        printf("\ncreat: 目录已满\n");
        iput(inode);
        return -1;
    }

    dir.direct[j].d_ino = inode->i_ino;
    dir.size++;

    inode->di_mode   = user[user_id].u_default_mode | DIFILE;
    inode->di_uid    = user[user_id].u_uid;
    inode->di_gid    = user[user_id].u_gid;
    inode->di_size   = 0;
    inode->di_number = 1;  /* 一个目录链接 */

    /* 分配 sys_ofile 槽位 */
    for (i = 0; i < SYSOPENFILE; i++)
        if (sys_ofile[i].f_count == 0) break;
    if (i == SYSOPENFILE) {
        iput(inode);
        return -1;
    }

    /* 分配用户文件描述符（从 1 开始，0 表示失败） */
    for (j = 1; j < NOFILE; j++)
        if (user[user_id].u_ofile[j] == SYSOPENFILE + 1) break;
    if (j == NOFILE) {
        iput(inode);
        return -1;
    }

    user[user_id].u_ofile[j] = (unsigned short)i;
    sys_ofile[i].f_flag  = FREAD | FWRITE;
    sys_ofile[i].f_count = 1;
    sys_ofile[i].f_off   = 0;
    sys_ofile[i].f_inode = inode;

    return j;
}

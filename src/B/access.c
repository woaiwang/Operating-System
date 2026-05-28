#include "filesys.h"

/*
 * access: 检查当前用户对指定 inode 是否具有所请求的访问权限
 *
 * 权限检查顺序：其他用户 → 同组用户 → 文件属主
 *
 * 原始 bug 修复：
 *  - 修复 "defualt" 拼写错误 → "default"
 *  - 修复 READ 分支中第二处检查：原来重复检查 ODIREAD，应为 GDIREAD
 */
static int check_access(struct inode *inode, unsigned short mode) {
    switch (mode) {
    case READ:
        if (inode->di_mode & ODIREAD) return 1;
        if ((inode->di_mode & GDIREAD) &&
            user[user_id].u_gid == inode->di_gid) return 1;
        if ((inode->di_mode & UDIREAD) &&
            user[user_id].u_uid == inode->di_uid) return 1;
        return 0;

    case WRITE:
        if (inode->di_mode & ODIWRITE) return 1;
        if ((inode->di_mode & GDIWRITE) &&
            user[user_id].u_gid == inode->di_gid) return 1;
        if ((inode->di_mode & UDIWRITE) &&
            user[user_id].u_uid == inode->di_uid) return 1;
        return 0;

    case EXICUTE:
        if (inode->di_mode & ODIEXICUTE) return 1;
        if ((inode->di_mode & GDIEXICUTE) &&
            user[user_id].u_gid == inode->di_gid) return 1;
        if ((inode->di_mode & UDIEXICUTE) &&
            user[user_id].u_uid == inode->di_uid) return 1;
        return 0;

    default:
        return 0;
    }
}

unsigned int access(unsigned int ino, unsigned int mode) {
    struct inode *inode;
    unsigned int result;

    inode = iget(ino);
    if (!inode) return 0;

    result = check_access(inode, (unsigned short)mode);
    iput(inode);
    return result;
}

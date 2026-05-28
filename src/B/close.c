#include "filesys.h"

/*
 * close: 关闭当前用户的文件描述符
 *
 * 原始 bug 修复：
 *  - 原来的代码不检查就递减 f_count，可能导致负值
 *    现在仅当 f_count > 0 时才递减
 *  - 增加参数合法性检查
 */
void close(int fd) {
    unsigned short sys_no;
    struct inode *inode;

    if (fd < 0 || fd >= NOFILE) return;

    sys_no = user[user_id].u_ofile[fd];
    if (sys_no == SYSOPENFILE + 1 || sys_no >= SYSOPENFILE) return;

    inode = sys_ofile[sys_no].f_inode;
    iput(inode);

    if (sys_ofile[sys_no].f_count > 0)
        sys_ofile[sys_no].f_count--;

    user[user_id].u_ofile[fd] = SYSOPENFILE + 1;
}

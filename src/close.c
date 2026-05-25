#include "filesys.h"

/*
 * close: close a file descriptor for the current user.
 *
 * Original bug fix:
 *  - The original decremented f_count to a potentially negative value
 *    without checking.  We only decrement if f_count > 0.
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

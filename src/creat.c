#include "filesys.h"
#include <stdio.h>

/*
 * creat: create a new file (or truncate an existing one) in the
 * current directory for the current user.
 *
 * Returns the per-user file descriptor on success, or -1 on failure.
 *
 * Original bug fixes:
 *  - Missing inode pointer initialization in "already existed" branch.
 *  - namei() now returns d_ino (inode number), not a directory index.
 *  - Fixed several uninitialized struct fields (f_count, etc.).
 *  - iname() now correctly copies the name into the directory entry.
 */
int creat(const char *name) {
    unsigned int di_ino;
    struct inode *inode;
    int i, j;

    di_ino = namei(name);

    if (di_ino != 0) {
        /* file already exists — truncate it */
        inode = iget(di_ino);
        if (!inode) return -1;

        if (!access(inode->i_ino, WRITE)) {
            printf("\ncreat: access denied\n");
            iput(inode);
            return -1;
        }

        /* free all blocks of the old file */
        for (i = 0; i < (int)(inode->di_size / BLOCKSIZ) + 1; i++)
            bfree(inode->di_addr[i]);

        inode->di_size = 0;

        /* reset offset on any open instances */
        for (i = 0; i < SYSOPENFILE; i++)
            if (sys_ofile[i].f_inode == inode)
                sys_ofile[i].f_off = 0;

        /* find/reuse a user file descriptor (start at 1, 0 = failure) */
        for (i = 1; i < NOFILE; i++) {
            if (user[user_id].u_ofile[i] == SYSOPENFILE + 1) {
                /* find a free sys_ofile slot */
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

    /* new file */
    inode = ialloc();
    if (!inode) return -1;

    j = (int)iname(name);  /* find empty slot, store name */
    if (j == 0 && dir.size >= DIRNUM) {
        printf("\ncreat: directory full\n");
        iput(inode);
        return -1;
    }

    dir.direct[j].d_ino = inode->i_ino;
    dir.size++;

    inode->di_mode   = user[user_id].u_default_mode | DIFILE;
    inode->di_uid    = user[user_id].u_uid;
    inode->di_gid    = user[user_id].u_gid;
    inode->di_size   = 0;
    inode->di_number = 1;  /* one directory link */

    /* allocate sys_ofile slot */
    for (i = 0; i < SYSOPENFILE; i++)
        if (sys_ofile[i].f_count == 0) break;
    if (i == SYSOPENFILE) {
        iput(inode);
        return -1;
    }

    /* allocate user fd (start at 1 so 0 = failure) */
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

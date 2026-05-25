#include "filesys.h"
#include <stdio.h>

/*
 * aopen: open a file for the current user.
 * Returns the per-user file descriptor index, or 0 on failure.
 *
 * Original bug fixes:
 *  - Reversed condition: "if (dinodeid != NULL)" should test for
 *    "if (dinodeid == 0)" (file not found).
 *  - u_ofile[j] was set to 1 instead of the sys_ofile index i.
 *  - Returns 0 (not NULL) on failure (correct for unsigned short).
 *  - namei() now returns d_ino, not a directory index.
 */
unsigned short aopen(const char *name, unsigned int mode) {
    unsigned int dinodeid;
    struct inode *inode;
    int i, j;

    dinodeid = namei(name);
    if (dinodeid == 0) {
        printf("\nfile does not exist!\n");
        return 0;
    }

    inode = iget(dinodeid);
    if (!inode) return 0;

    if (!access(inode->i_ino, mode)) {
        printf("\nfile open access denied!\n");
        iput(inode);
        return 0;
    }

    /* allocate a sys_ofile slot */
    for (i = 1; i < SYSOPENFILE; i++)
        if (sys_ofile[i].f_count == 0) break;
    if (i == SYSOPENFILE) {
        printf("\nsystem open file table full\n");
        iput(inode);
        return 0;
    }
    sys_ofile[i].f_inode = inode;
    sys_ofile[i].f_flag  = (char)mode;
    sys_ofile[i].f_count = 1;
    sys_ofile[i].f_off   = (mode & FAPPEND) ? inode->di_size : 0;

    /* allocate a user file descriptor (start at 1 so fd=0 means failure) */
    for (j = 1; j < NOFILE; j++)
        if (user[user_id].u_ofile[j] == SYSOPENFILE + 1) break;
    if (j == NOFILE) {
        printf("\nuser open file table full!\n");
        sys_ofile[i].f_count = 0;
        iput(inode);
        return 0;
    }
    user[user_id].u_ofile[j] = (unsigned short)i;

    /* if append mode, truncate the file */
    if (mode & FAPPEND) {
        int k;
        for (k = 0; k < (int)(inode->di_size / BLOCKSIZ) + 1; k++)
            bfree(inode->di_addr[k]);
        inode->di_size = 0;
        inode->di_number = 0;
    }

    return (unsigned short)j;
}

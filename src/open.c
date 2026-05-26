#include "filesys.h"
#include <stdio.h>

unsigned short aopen(int user_id, const char *filename, unsigned short openmode) {
    unsigned int dinodeid;
    struct inode *inode;
    int i, j;
    
    dinodeid = namei(filename);
    if (dinodeid == (unsigned int)-1) {
        printf("\nFile does not exist!\n");
        return 0;
    }
    
    inode = iget(dinodeid);
    if (!access(user_id, inode, (openmode & FREAD) ? READ : WRITE)) {
        printf("\nFile open access denied!\n");
        iput(inode);
        return 0;
    }
    
    for (i = 0; i < SYSOPENFILE; i++) {
        if (sys_ofile[i].f_count == 0) {
            break;
        }
    }
    if (i == SYSOPENFILE) {
        printf("\nSystem open file table is full\n");
        iput(inode);
        return 0;
    }
    
    sys_ofile[i].f_inode = inode;
    sys_ofile[i].f_flag = openmode;
    sys_ofile[i].f_count = 1;
    sys_ofile[i].f_off = (openmode & FAPPEND) ? inode->di_size : 0;
    
    for (j = 0; j < NOFILE; j++) {
        if (user[user_id].u_ofile[j] == SYSOPENFILE + 1) {
            break;
        }
    }
    if (j == NOFILE) {
        printf("\nUser open file table is full\n");
        sys_ofile[i].f_count = 0;
        iput(inode);
        return 0;
    }
    
    user[user_id].u_ofile[j] = i;
    
    return j;
}
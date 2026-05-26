#include "filesys.h"
#include <stdio.h>

void close(int user_id, unsigned short cfd) {
    struct inode *inode;
    unsigned int sys_fd;
    
    if (cfd >= NOFILE) {
        printf("\nInvalid file descriptor\n");
        return;
    }
    
    sys_fd = user[user_id].u_ofile[cfd];
    if (sys_fd == SYSOPENFILE + 1) {
        printf("\nFile not open\n");
        return;
    }
    
    inode = sys_ofile[sys_fd].f_inode;
    iput(inode);
    sys_ofile[sys_fd].f_count--;
    user[user_id].u_ofile[cfd] = SYSOPENFILE + 1;
}
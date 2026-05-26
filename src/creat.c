#include "filesys.h"
#include <stdio.h>
#include <string.h>

unsigned short creat(int user_id, const char *filename, unsigned short mode) {
    unsigned int di_ino;
    struct inode *inode;
    int i, j;
    
    di_ino = namei(filename);
    if (di_ino != (unsigned int)-1) {
        inode = iget(di_ino);
        if (!access(user_id, inode, WRITE)) {
            iput(inode);
            printf("\nCreate access not allowed\n");
            return (unsigned short)-1;
        }
        
        for (i = 0; i < inode->di_size / BLOCKSIZ + 1; i++) {
            if (inode->di_addr[i] != 0) {
                bfree(inode->di_addr[i]);
            }
        }
        
        for (i = 0; i < SYSOPENFILE; i++) {
            if (sys_ofile[i].f_inode == inode) {
                sys_ofile[i].f_off = 0;
            }
        }
        
        inode->di_size = 0;
        
        for (j = 0; j < NOFILE; j++) {
            if (user[user_id].u_ofile[j] == SYSOPENFILE + 1) {
                for (i = 0; i < SYSOPENFILE; i++) {
                    if (sys_ofile[i].f_count == 0) {
                        user[user_id].u_ofile[j] = i;
                        sys_ofile[i].f_flag = mode;
                        sys_ofile[i].f_count = 1;
                        sys_ofile[i].f_off = 0;
                        sys_ofile[i].f_inode = inode;
                        return j;
                    }
                }
                return (unsigned short)-1;
            }
        }
        iput(inode);
        return (unsigned short)-1;
    } else {
        inode = ialloc();
        unsigned short di_ith = iname(filename);
        if (di_ith == (unsigned short)-1) {
            iput(inode);
            return (unsigned short)-1;
        }
        dir.size++;
        
        dir.direct[di_ith].d_ino = inode->i_ino;
        
        inode->di_mode = user[user_id].u_default_mode | DIFILE;
        inode->di_uid = user[user_id].u_uid;
        inode->di_gid = user[user_id].u_gid;
        inode->di_size = 0;
        inode->di_number = 1;
        
        for (i = 0; i < SYSOPENFILE; i++) {
            if (sys_ofile[i].f_count == 0) {
                break;
            }
        }
        if (i == SYSOPENFILE) {
            iput(inode);
            printf("\nSystem open file table is full\n");
            return (unsigned short)-1;
        }
        
        for (j = 0; j < NOFILE; j++) {
            if (user[user_id].u_ofile[j] == SYSOPENFILE + 1) {
                break;
            }
        }
        if (j == NOFILE) {
            iput(inode);
            printf("\nUser open file table is full\n");
            return (unsigned short)-1;
        }
        
        user[user_id].u_ofile[j] = i;
        sys_ofile[i].f_flag = mode;
        sys_ofile[i].f_count = 1;
        sys_ofile[i].f_off = 0;
        sys_ofile[i].f_inode = inode;
        
        return j;
    }
}
#include "filesys.h"
#include <stdio.h>
#include <string.h>

int login(unsigned short uid, char *passwd) {
    int i, j;
    
    for (i = 0; i < PWDNUM; i++) {
        if ((uid == pwd[i].p_uid) && (!strcmp(passwd, pwd[i].password))) {
            for (j = 0; j < USERNUM; j++) {
                if (user[j].u_uid == 0) break;
            }
            if (j == USERNUM) {
                printf("\nToo many users in the system, please wait\n");
                return 0;
            }
            user[j].u_uid = uid;
            user[j].u_gid = pwd[i].p_gid;
            user[j].u_default_mode = DEFAULTMODE;
            break;
        }
    }
    
    if (i == PWDNUM) {
        printf("\nIncorrect password\n");
        return 0;
    } else {
        return 1;
    }
}

int logout(unsigned short uid) {
    int i, j, sys_no;
    struct inode *inode;
    
    for (i = 0; i < USERNUM; i++) {
        if (uid == user[i].u_uid) break;
    }
    
    if (i == USERNUM) {
        printf("\nNo such user\n");
        return 0;
    }
    
    for (j = 0; j < NOFILE; j++) {
        if (user[i].u_ofile[j] != SYSOPENFILE + 1) {
            sys_no = user[i].u_ofile[j];
            inode = sys_ofile[sys_no].f_inode;
            iput(inode);
            sys_ofile[sys_no].f_count--;
            user[i].u_ofile[j] = SYSOPENFILE + 1;
        }
    }
    
    user[i].u_uid = 0;
    user[i].u_gid = 0;
    return 1;
}
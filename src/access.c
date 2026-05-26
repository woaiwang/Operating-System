#include "filesys.h"
#include <stdio.h>

unsigned int access(int user_id, struct inode *inode, unsigned short mode) {
    if (inode == NULL) return 0;
    
    switch (mode) {
        case READ:
            if (inode->di_mode & ODIREAD) return 1;
            if ((inode->di_mode & GDIREAD) && (user[user_id].u_gid == inode->di_gid)) return 1;
            if ((inode->di_mode & UDIREAD) && (user[user_id].u_uid == inode->di_uid)) return 1;
            return 0;
            
        case WRITE:
            if (inode->di_mode & ODIWRITE) return 1;
            if ((inode->di_mode & GDIWRITE) && (user[user_id].u_gid == inode->di_gid)) return 1;
            if ((inode->di_mode & UDIWRITE) && (user[user_id].u_uid == inode->di_uid)) return 1;
            return 0;
            
        case EXICUTE:
            if (inode->di_mode & ODIEXICUTE) return 1;
            if ((inode->di_mode & GDIEXICUTE) && (user[user_id].u_gid == inode->di_gid)) return 1;
            if ((inode->di_mode & UDIEXICUTE) && (user[user_id].u_uid == inode->di_uid)) return 1;
            return 0;
            
        default:
            return 0;
    }
}
#include "filesys.h"

/*
 * access: check whether the current user has the requested access mode
 * to the given inode.
 *
 * Permission check order: other, then group, then user.
 *
 * Original bug fixes:
 *  - Fixed "defualt" typo → "default".
 *  - Fixed READ case: the second check was testing ODIREAD again
 *    instead of GDIREAD.
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

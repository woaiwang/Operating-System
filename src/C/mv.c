#include "filesys.h"
#include <stdio.h>
#include <string.h>

void mv(const char *src, const char *dst) {
    unsigned int src_ino, dst_ino;
    struct inode *src_inode;
    int i;

    src_ino = namei(src);
    if (src_ino == 0) {
        printf("mv: cannot stat '%s': No such file or directory\n", src);
        return;
    }

    src_inode = iget(src_ino);
    if (!src_inode) {
        printf("mv: cannot get inode for '%s'\n", src);
        return;
    }

    dst_ino = namei(dst);
    if (dst_ino != 0) {
        printf("mv: '%s' already exists\n", dst);
        iput(src_inode);
        return;
    }

    for (i = 0; i < dir.size; i++) {
        if (dir.direct[i].d_ino == src_ino) {
            strncpy(dir.direct[i].d_name, dst, DIRSIZ);
            dir.direct[i].d_name[DIRSIZ - 1] = '\0';
            break;
        }
    }

    iput(src_inode);
    printf("mv: renamed '%s' to '%s'\n", src, dst);
}

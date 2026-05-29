#include "filesys.h"
#include <stdio.h>
#include <string.h>

/*
 * ln: create a hard link to an existing file.
 */
void ln(const char *src, const char *dst) {
    unsigned int src_ino, dst_ino;
    struct inode *src_inode;
    int dirpos;

    if (!src || !dst || !src[0] || !dst[0]) {
        printf("Usage: ln <src> <dst>\n");
        return;
    }

    src_ino = namei(src);
    if (src_ino == 0) {
        printf("ln: failed to access '%s': No such file or directory\n", src);
        return;
    }

    src_inode = iget(src_ino);
    if (!src_inode) {
        printf("ln: cannot get inode for '%s'\n", src);
        return;
    }

    if (src_inode->di_mode & DIDIR) {
        printf("ln: '%s': hard link not allowed for directory\n", src);
        iput(src_inode);
        return;
    }

    dst_ino = namei(dst);
    if (dst_ino != 0) {
        printf("ln: failed to create hard link '%s': File exists\n", dst);
        iput(src_inode);
        return;
    }

    dirpos = iname(dst);
    dir.direct[dirpos].d_ino = src_inode->i_ino;
    dir.size++;

    src_inode->di_number++;

    sync_dir();
    iput(src_inode);

    printf("ln: created hard link '%s' -> '%s' (inode: %u, links: %u)\n", dst, src, src_inode->i_ino, src_inode->di_number);
}

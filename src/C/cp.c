#include "filesys.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cp(const char *src, const char *dst) {
    unsigned int src_ino, dst_ino;
    struct inode *src_inode, *dst_inode;
    char *buf;
    unsigned int size;
    unsigned short src_fd;
    int dst_fd;

    src_ino = namei(src);
    if (src_ino == 0) {
        printf("cp: cannot stat '%s': No such file or directory\n", src);
        return;
    }

    src_inode = iget(src_ino);
    if (!src_inode) {
        printf("cp: cannot get inode for '%s'\n", src);
        return;
    }

    if (!(src_inode->di_mode & DIFILE)) {
        printf("cp: '%s' is a directory (not supported)\n", src);
        iput(src_inode);
        return;
    }

    dst_ino = namei(dst);
    if (dst_ino != 0) {
        printf("cp: '%s' already exists\n", dst);
        iput(src_inode);
        return;
    }

    size = src_inode->di_size;
    buf = (char *)malloc(size);
    if (!buf) {
        printf("cp: memory allocation failed\n");
        iput(src_inode);
        return;
    }

    src_fd = aopen(src, FREAD);
    if (src_fd == 0) {
        printf("cp: cannot open '%s' for reading\n", src);
        free(buf);
        iput(src_inode);
        return;
    }

    fs_read(src_fd, buf, size);
    close(src_fd);

    dst_fd = creat(dst);
    if (dst_fd < 0) {
        printf("cp: cannot create '%s'\n", dst);
        free(buf);
        iput(src_inode);
        return;
    }

    fs_write(dst_fd, buf, size);
    close(dst_fd);

    dst_inode = iget(namei(dst));
    if (dst_inode) {
        dst_inode->di_mode = src_inode->di_mode;
        dst_inode->di_uid = src_inode->di_uid;
        dst_inode->di_gid = src_inode->di_gid;
        iput(dst_inode);
    }

    free(buf);
    iput(src_inode);

    printf("cp: copied '%s' to '%s'\n", src, dst);
}

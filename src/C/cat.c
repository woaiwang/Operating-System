#include "filesys.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cat(const char *name) {
    unsigned int ino;
    struct inode *inode;
    char *buf;
    unsigned int size;

    ino = namei(name);
    if (ino == 0) {
        printf("cat: %s: No such file or directory\n", name);
        return;
    }

    inode = iget(ino);
    if (!inode) {
        printf("cat: %s: Cannot get inode\n", name);
        return;
    }

    if (!(inode->di_mode & DIFILE)) {
        printf("cat: %s: Is a directory\n", name);
        iput(inode);
        return;
    }

    size = inode->di_size;
    if (size == 0) {
        iput(inode);
        return;
    }

    buf = (char *)malloc(size + 1);
    if (!buf) {
        printf("cat: memory allocation failed\n");
        iput(inode);
        return;
    }

    unsigned short fd = aopen(name, FREAD);
    if (fd == 0) {
        printf("cat: cannot open %s\n", name);
        free(buf);
        iput(inode);
        return;
    }

    fs_read(fd, buf, size);
    buf[size] = '\0';
    printf("%s", buf);

    close(fd);
    free(buf);
    iput(inode);
}

#include "filesys.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void grep(const char *pattern, const char *filename) {
    unsigned int ino;
    struct inode *inode;
    char *buf;
    unsigned int size;
    unsigned short fd;
    char *line_start, *line_end;

    ino = namei(filename);
    if (ino == 0) {
        printf("grep: %s: No such file or directory\n", filename);
        return;
    }

    inode = iget(ino);
    if (!inode) {
        printf("grep: cannot get inode for %s\n", filename);
        return;
    }

    if (!(inode->di_mode & DIFILE)) {
        printf("grep: %s: Is a directory\n", filename);
        iput(inode);
        return;
    }

    size = inode->di_size;
    buf = (char *)malloc(size + 1);
    if (!buf) {
        printf("grep: memory allocation failed\n");
        iput(inode);
        return;
    }

    fd = aopen(filename, FREAD);
    if (fd == 0) {
        printf("grep: cannot open %s\n", filename);
        free(buf);
        iput(inode);
        return;
    }

    fs_read(fd, buf, size);
    buf[size] = '\0';
    close(fd);

    line_start = buf;
    while (line_start < buf + size) {
        line_end = strchr(line_start, '\n');
        if (!line_end) line_end = buf + size;
        
        *line_end = '\0';
        if (strstr(line_start, pattern) != NULL) {
            printf("%s\n", line_start);
        }
        
        *line_end = '\n';
        line_start = line_end + 1;
    }

    free(buf);
    iput(inode);
}

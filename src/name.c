#include "filesys.h"
#include <stdio.h>
#include <string.h>

unsigned int namei(const char *name) {
    int i;
    for (i = 0; i < dir.size; i++) {
        if (dir.direct[i].d_ino != 0 && strcmp(dir.direct[i].d_name, name) == 0) {
            return dir.direct[i].d_ino;
        }
    }
    return (unsigned int)-1;
}

unsigned short iname(const char *name) {
    int i;
    for (i = 0; i < DIRNUM; i++) {
        if (dir.direct[i].d_ino == 0) {
            strncpy(dir.direct[i].d_name, name, DIRSIZ - 1);
            dir.direct[i].d_name[DIRSIZ - 1] = '\0';
            return i;
        }
    }
    printf("\nCurrent directory is full!\n");
    return (unsigned short)-1;
}
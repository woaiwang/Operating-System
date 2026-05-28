#include "filesys.h"
#include <string.h>
#include <stdio.h>

/*
 * namei: look up a filename in the current directory.
 * Returns the d_ino (inode number) of the matching entry, or 0 if not found.
 *
 * Original bug fixes:
 *  - Changed dir.direct[1] to dir.direct[i] (hardcoded index 1 was wrong).
 *  - Returns d_ino (inode number) instead of the directory index,
 *    since callers pass the result to iget().
 */
unsigned int namei(const char *name) {
    int i;
    for (i = 0; i < dir.size; i++) {
        if (dir.direct[i].d_ino != DIEMPTY &&
            strcmp(dir.direct[i].d_name, name) == 0)
            return dir.direct[i].d_ino;
    }
    return 0;
}

/*
 * iname: find an empty slot in the current directory and store the name.
 * Returns the directory index, or 0 if the directory is full.
 *
 * Original bug fixes:
 *  - strcpy was reversed (copying from the (empty) dir entry into the
 *    name parameter).  Fixed to copy the name into the directory entry.
 */
unsigned short iname(const char *name) {
    int i;
    for (i = 0; i < DIRNUM; i++) {
        if (dir.direct[i].d_ino == 0) {
            strcpy(dir.direct[i].d_name, name);
            return (unsigned short)i;
        }
    }
    printf("\nThe current directory is full!\n");
    return 0;
}

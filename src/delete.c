#include "filesys.h"
#include <string.h>
#include <stdio.h>

/*
 * delete: remove a file from the current directory.
 *
 * Clears the directory entry, then decrements the link count.
 * When it reaches 0, iput() will free the blocks and the inode.
 *
 * Original bug fixes:
 *  - Added NULL check for namei result.
 *  - Added directory entry cleanup (missing in original — the
 *    name would remain in the directory after deletion).
 */
void delete(const char *name) {
    unsigned int dinodeid;
    struct inode *inode;
    int i;

    dinodeid = namei(name);
    if (dinodeid == 0) {
        printf("\ndelete: file not found\n");
        return;
    }

    /* clear the directory entry */
    for (i = 0; i < dir.size; i++) {
        if (dir.direct[i].d_ino == dinodeid &&
            strcmp(dir.direct[i].d_name, name) == 0) {
            memset(&dir.direct[i], 0, sizeof(struct direct));
            break;
        }
    }

    inode = iget(dinodeid);
    if (!inode) return;

    if (inode->di_number > 0)
        inode->di_number--;
    iput(inode);
}

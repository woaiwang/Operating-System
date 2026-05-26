#include "filesys.h"
#include <stdio.h>

void delete(const char *filename) {
    unsigned int dinodeid;
    struct inode *inode;
    int i;
    
    dinodeid = namei(filename);
    if (dinodeid == (unsigned int)-1) {
        printf("\nFile %s does not exist\n", filename);
        return;
    }
    
    inode = iget(dinodeid);
    if (!(inode->di_mode & DIFILE)) {
        printf("\n%s is not a file\n", filename);
        iput(inode);
        return;
    }
    
    for (i = 0; i < inode->di_size / BLOCKSIZ + 1; i++) {
        if (inode->di_addr[i] != 0) {
            bfree(inode->di_addr[i]);
        }
    }
    
    inode->di_number = 0;
    dir.direct[dinodeid].d_ino = 0;
    dir.size--;
    
    iput(inode);
}
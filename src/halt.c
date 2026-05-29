#include "filesys.h"
#include <stdlib.h>
#include <stdio.h>

void halt(void) {
    debug_log("halt: shutting down...\n");

    /* Sync memory directory buffer to disk */
    sync_dir();

    /* flush current directory inode to disk */
    if (cur_path_inode) {
        iput(cur_path_inode);
        cur_path_inode = NULL;
    }

    if (fd) {
        /* write back superblock to block 1 */
        if (fseek(fd, BLOCKSIZ, SEEK_SET) == 0) {
            if (fwrite(&filsys, sizeof(struct filsys), 1, fd) != 1)
                printf("halt WARNING: superblock write failed\n");
        }
        if (fclose(fd) != 0)
            printf("halt WARNING: fclose failed\n");
        fd = NULL;
    }

    printf("Good Bye. See You Next Time.\n");
    fflush(stdout);
    exit(0);
}

#include "filesys.h"
#include <stdio.h>
#include <stdlib.h>

void halt(void) {
    int i, j;
    
    chdir("..");
    iput(cur_path_inode);
    
    for (i = 0; i < USERNUM; i++) {
        if (user[i].u_uid != 0) {
            for (j = 0; j < NOFILE; j++) {
                if (user[i].u_ofile[j] != SYSOPENFILE + 1) {
                    close(i, j);
                    user[i].u_ofile[j] = SYSOPENFILE + 1;
                }
            }
        }
    }
    
    fseek(fd, BLOCKSIZ, SEEK_SET);
    fwrite(&filsys, 1, sizeof(struct filsys), fd);
    
    fclose(fd);
    
    printf("\nGood Bye. See You Next Time.\n");
    exit(0);
}
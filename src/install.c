#include "filesys.h"
#include <stdio.h>
#include <string.h>

void install(void) {
    int i, j;
    
    fd = fopen("filesystem", "r+b");
    if (fd == NULL) {
        printf("\nFilesystem file not found, formatting...\n");
        format();
        fd = fopen("filesystem", "r+b");
        if (fd == NULL) {
            printf("\nCannot open filesystem\n");
            return;
        }
    }
    
    fseek(fd, BLOCKSIZ, SEEK_SET);
    fread(&filsys, 1, sizeof(struct filsys), fd);
    
    for (i = 0; i < NHINO; i++) {
        hinode[i].i_forw = NULL;
    }
    
    for (i = 0; i < SYSOPENFILE; i++) {
        sys_ofile[i].f_count = 0;
        sys_ofile[i].f_inode = NULL;
    }
    
    for (i = 0; i < USERNUM; i++) {
        user[i].u_uid = 0;
        user[i].u_gid = 0;
        for (j = 0; j < NOFILE; j++) {
            user[i].u_ofile[j] = SYSOPENFILE + 1;
        }
    }
    
    cur_path_inode = iget(1);
    dir.size = cur_path_inode->di_size / (DIRSIZ + 2);
    
    for (i = 0; i < DIRNUM; i++) {
        memset(dir.direct[i].d_name, ' ', DIRSIZ);
        dir.direct[i].d_name[DIRSIZ - 1] = '\0';
        dir.direct[i].d_ino = 0;
    }
    
    for (i = 0; i < dir.size / (BLOCKSIZ / (DIRSIZ + 2)) + 1; i++) {
        if (i < cur_path_inode->di_size / BLOCKSIZ + 1) {
            fseek(fd, DATASTART + BLOCKSIZ * cur_path_inode->di_addr[i], SEEK_SET);
            fread(&dir.direct[(BLOCKSIZ / (DIRSIZ + 2)) * i], 1, BLOCKSIZ, fd);
        }
    }
    
    fseek(fd, DINODESTART, SEEK_SET);
    fread(pwd, 1, sizeof(struct pwd) * PWDNUM, fd);
    
    printf("\nFilesystem installed successfully!\n");
}
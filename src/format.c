#include "filesys.h"
#include <stdio.h>
#include <string.h>

void format(void) {
    int i, j;
    unsigned char zero_block[BLOCKSIZ] = {0};
    struct dinode din;
    struct direct dir_entry;
    unsigned int root_block;
    
    fd = fopen("filesystem", "w+b");
    if (fd == NULL) {
        printf("\nCannot create filesystem file\n");
        return;
    }
    
    for (i = 0; i < FILEBLK; i++) {
        fwrite(zero_block, 1, BLOCKSIZ, fd);
    }
    
    filsys.s_isize = DINODEBLK;
    filsys.s_fsize = FILEBLK;
    filsys.s_nfree = FILEBLK - (2 + DINODEBLK);
    filsys.s_pfree = NICFREE - 1;
    
    for (i = 0; i < NICFREE && i < filsys.s_nfree; i++) {
        filsys.s_free[NICFREE - 1 - i] = 2 + DINODEBLK + i;
    }
    filsys.s_pfree = NICFREE - i;
    
    filsys.s_ninode = DINODEBLK * (BLOCKSIZ / DINODESIZ);
    filsys.s_pinode = 0;
    filsys.s_rinode = 0;
    for (i = 0; i < NICINOD; i++) {
        filsys.s_inode[i] = i;
    }
    filsys.s_pinode = NICINOD;
    filsys.s_fmod = SUPDATE;
    
    fseek(fd, BLOCKSIZ, SEEK_SET);
    fwrite(&filsys, 1, sizeof(struct filsys), fd);
    
    memset(&din, 0, sizeof(struct dinode));
    din.di_number = 1;
    din.di_mode = DIDIR | DEFAULTMODE;
    din.di_uid = 0;
    din.di_gid = 0;
    din.di_size = 2 * (DIRSIZ + 2);
    
    root_block = balloc();
    din.di_addr[0] = root_block;
    
    fseek(fd, DINODESTART + 1 * DINODESIZ, SEEK_SET);
    fwrite(&din, 1, sizeof(struct dinode), fd);
    
    memset(&dir_entry, 0, sizeof(struct direct));
    strcpy(dir_entry.d_name, ".");
    dir_entry.d_ino = 1;
    
    fseek(fd, DATASTART + root_block * BLOCKSIZ, SEEK_SET);
    fwrite(&dir_entry, 1, sizeof(struct direct), fd);
    
    strcpy(dir_entry.d_name, "..");
    dir_entry.d_ino = 1;
    fwrite(&dir_entry, 1, sizeof(struct direct), fd);
    
    pwd[0].p_uid = 2118;
    pwd[0].p_gid = 1;
    strcpy(pwd[0].password, "abcd");
    
    fseek(fd, DINODESTART, SEEK_SET);
    fwrite(pwd, 1, sizeof(struct pwd) * PWDNUM, fd);
    
    fclose(fd);
    printf("\nFormat completed successfully!\n");
}
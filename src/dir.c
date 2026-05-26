#include "filesys.h"
#include <stdio.h>
#include <string.h>

void _dir(void) {
    unsigned int di_mode;
    int i, j, one;
    struct inode *temp_inode;
    
    printf("\nCURRENT DIRECTORY ..\n");
    for (i = 0; i < dir.size; i++) {
        if (dir.direct[i].d_ino != DIEMPTY) {
            printf("%14s", dir.direct[i].d_name);
            temp_inode = iget(dir.direct[i].d_ino);
            di_mode = temp_inode->di_mode;
            
            for (j = 0; j < 9; j++) {
                one = di_mode % 2;
                di_mode = di_mode / 2;
                if (one) printf("x");
                else printf("-");
            }
            
            if (temp_inode->di_mode & DIFILE) {
                printf(" %d\n", temp_inode->di_size);
                printf("block chain:");
                for (j = 0; j < temp_inode->di_size / BLOCKSIZ + 1; j++) {
                    printf("%4d", temp_inode->di_addr[j]);
                }
                printf("\n");
            } else {
                printf(" <dir>\n");
            }
            iput(temp_inode);
        }
    }
}

void mkdir(char *dirname) {
    int dirid, dirpos;
    struct inode *inode;
    struct direct buf[BLOCKSIZ / (DIRSIZ + 2)];
    unsigned int block;
    
    dirid = namei(dirname);
    if (dirid != (unsigned int)-1) {
        inode = iget(dirid);
        if (inode->di_mode & DIDIR) {
            printf("\n%s directory already existed!\n", dirname);
        } else {
            printf("\n%s is a file name, can't create a dir with the same name\n", dirname);
        }
        iput(inode);
        return;
    }
    
    dirpos = iname(dirname);
    inode = ialloc();
    dir.direct[dirpos].d_ino = inode->i_ino;
    dir.size++;
    
    strcpy(buf[0].d_name, ".");
    buf[0].d_ino = inode->i_ino;
    strcpy(buf[1].d_name, "..");
    buf[1].d_ino = cur_path_inode->i_ino;
    
    block = balloc();
    fseek(fd, DATASTART + block * BLOCKSIZ, SEEK_SET);
    fwrite(buf, 1, BLOCKSIZ, fd);
    
    inode->di_size = 2 * (DIRSIZ + 2);
    inode->di_number = 1;
    inode->di_mode = user[user_id].u_default_mode | DIDIR;
    inode->di_uid = user[user_id].u_uid;
    inode->di_gid = user[user_id].u_gid;
    inode->di_addr[0] = block;
    
    iput(inode);
}

void chdir(char *dirname) {
    unsigned int dirid;
    struct inode *inode;
    unsigned short block;
    int i, j;
    
    dirid = namei(dirname);
    if (dirid == (unsigned int)-1) {
        printf("\n%s does not exist\n", dirname);
        return;
    }
    
    inode = iget(dirid);
    if (!access(user_id, inode, user[user_id].u_default_mode)) {
        printf("\nNo access to directory %s\n", dirname);
        iput(inode);
        return;
    }
    
    for (i = 0; i < dir.size; i++) {
        for (j = 0; j < DIRNUM; j++) {
            if (dir.direct[j].d_ino == 0) break;
        }
        memcpy(&dir.direct[i], &dir.direct[j], DIRSIZ + 2);
        dir.direct[j].d_ino = 0;
    }
    
    for (i = 0; i < cur_path_inode->di_size / BLOCKSIZ + 1; i++) {
        if (cur_path_inode->di_addr[i] != 0) {
            bfree(cur_path_inode->di_addr[i]);
        }
    }
    
    for (i = 0; i < dir.size; i += BLOCKSIZ / (DIRSIZ + 2)) {
        block = balloc();
        cur_path_inode->di_addr[i / (BLOCKSIZ / (DIRSIZ + 2))] = block;
        fseek(fd, DATASTART + block * BLOCKSIZ, SEEK_SET);
        fwrite(&dir.direct[i], 1, BLOCKSIZ, fd);
    }
    
    cur_path_inode->di_size = dir.size * (DIRSIZ + 2);
    iput(cur_path_inode);
    cur_path_inode = inode;
    
    j = 0;
    for (i = 0; i < inode->di_size / BLOCKSIZ + 1; i++) {
        fseek(fd, DATASTART + inode->di_addr[i] * BLOCKSIZ, SEEK_SET);
        fread(&dir.direct[j], 1, BLOCKSIZ, fd);
        j += BLOCKSIZ / (DIRSIZ + 2);
    }
    
    dir.size = inode->di_size / (DIRSIZ + 2);
}
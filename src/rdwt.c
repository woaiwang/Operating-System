#include "filesys.h"
#include <stdio.h>

unsigned int read(unsigned short ufd, char *buf, unsigned int size) {
    unsigned long off;
    int block, block_off, i;
    struct inode *inode;
    char *temp_buf;
    unsigned int sys_fd;
    
    if (ufd >= NOFILE) {
        printf("\nInvalid file descriptor\n");
        return 0;
    }
    
    sys_fd = user[user_id].u_ofile[ufd];
    if (sys_fd == SYSOPENFILE + 1) {
        printf("\nFile not open\n");
        return 0;
    }
    
    inode = sys_ofile[sys_fd].f_inode;
    if (!(sys_ofile[sys_fd].f_flag & FREAD)) {
        printf("\nFile not opened for reading\n");
        return 0;
    }
    
    temp_buf = buf;
    off = sys_ofile[sys_fd].f_off;
    if ((off + size) > inode->di_size) {
        size = inode->di_size - off;
    }
    if (size == 0) return 0;
    
    block_off = off % BLOCKSIZ;
    block = off / BLOCKSIZ;
    
    if (block_off + size <= BLOCKSIZ) {
        fseek(fd, DATASTART + inode->di_addr[block] * BLOCKSIZ + block_off, SEEK_SET);
        fread(buf, 1, size, fd);
        sys_ofile[sys_fd].f_off += size;
        return size;
    }
    
    fseek(fd, DATASTART + inode->di_addr[block] * BLOCKSIZ + block_off, SEEK_SET);
    fread(temp_buf, 1, BLOCKSIZ - block_off, fd);
    temp_buf += BLOCKSIZ - block_off;
    
    for (i = 0; i < (size - (BLOCKSIZ - block_off)) / BLOCKSIZ; i++) {
        fseek(fd, DATASTART + inode->di_addr[block + 1 + i] * BLOCKSIZ, SEEK_SET);
        fread(temp_buf, 1, BLOCKSIZ, fd);
        temp_buf += BLOCKSIZ;
    }
    
    block_off = (size - (BLOCKSIZ - block_off)) % BLOCKSIZ;
    if (block_off > 0) {
        fseek(fd, DATASTART + inode->di_addr[block + 1 + i] * BLOCKSIZ, SEEK_SET);
        fread(temp_buf, 1, block_off, fd);
    }
    
    sys_ofile[sys_fd].f_off += size;
    return size;
}

unsigned int write(unsigned short ufd, const char *buf, unsigned int size) {
    unsigned long off;
    int block, block_off, i;
    struct inode *inode;
    const char *temp_buf;
    unsigned int sys_fd;
    
    if (ufd >= NOFILE) {
        printf("\nInvalid file descriptor\n");
        return 0;
    }
    
    sys_fd = user[user_id].u_ofile[ufd];
    if (sys_fd == SYSOPENFILE + 1) {
        printf("\nFile not open\n");
        return 0;
    }
    
    inode = sys_ofile[sys_fd].f_inode;
    if (!(sys_ofile[sys_fd].f_flag & FWRITE)) {
        printf("\nFile not opened for writing\n");
        return 0;
    }
    
    temp_buf = buf;
    off = sys_ofile[sys_fd].f_off;
    block_off = off % BLOCKSIZ;
    block = off / BLOCKSIZ;
    
    if (block_off + size <= BLOCKSIZ) {
        if (inode->di_addr[block] == 0) {
            inode->di_addr[block] = balloc();
        }
        fseek(fd, DATASTART + inode->di_addr[block] * BLOCKSIZ + block_off, SEEK_SET);
        fwrite(buf, 1, size, fd);
        sys_ofile[sys_fd].f_off += size;
        if (off + size > inode->di_size) {
            inode->di_size = off + size;
        }
        return size;
    }
    
    if (inode->di_addr[block] == 0) {
        inode->di_addr[block] = balloc();
    }
    fseek(fd, DATASTART + inode->di_addr[block] * BLOCKSIZ + block_off, SEEK_SET);
    fwrite(temp_buf, 1, BLOCKSIZ - block_off, fd);
    temp_buf += BLOCKSIZ - block_off;
    
    for (i = 0; i < (size - (BLOCKSIZ - block_off)) / BLOCKSIZ; i++) {
        inode->di_addr[block + 1 + i] = balloc();
        fseek(fd, DATASTART + inode->di_addr[block + 1 + i] * BLOCKSIZ, SEEK_SET);
        fwrite(temp_buf, 1, BLOCKSIZ, fd);
        temp_buf += BLOCKSIZ;
    }
    
    block_off = (size - (BLOCKSIZ - block_off)) % BLOCKSIZ;
    if (block_off > 0) {
        inode->di_addr[block + 1 + i] = balloc();
        fseek(fd, DATASTART + inode->di_addr[block + 1 + i] * BLOCKSIZ, SEEK_SET);
        fwrite(temp_buf, 1, block_off, fd);
    }
    
    sys_ofile[sys_fd].f_off += size;
    if (off + size > inode->di_size) {
        inode->di_size = off + size;
    }
    return size;
}
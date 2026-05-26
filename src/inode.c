#include "filesys.h"
#include <stdio.h>
#include <stdlib.h>

struct inode *iget(unsigned int dinodeid) {
    int existed = 0, inodeid;
    long addr;
    struct inode *temp, *newinode;
    
    inodeid = dinodeid % NHINO;
    
    if (hinode[inodeid].i_forw == NULL) {
        existed = 0;
    } else {
        temp = hinode[inodeid].i_forw;
        while (temp != NULL) {
            if (temp->i_ino == dinodeid) {
                existed = 1;
                temp->i_count++;
                return temp;
            }
            temp = temp->i_forw;
        }
    }
    
    addr = DINODESTART + dinodeid * DINODESIZ;
    newinode = (struct inode *)malloc(sizeof(struct inode));
    fseek(fd, addr, SEEK_SET);
    fread(&(newinode->di_number), DINODESIZ, 1, fd);
    
    newinode->i_forw = hinode[inodeid].i_forw;
    newinode->i_back = NULL;
    if (hinode[inodeid].i_forw != NULL) {
        hinode[inodeid].i_forw->i_back = newinode;
    }
    hinode[inodeid].i_forw = newinode;
    
    newinode->i_count = 1;
    newinode->i_flag = 0;
    newinode->i_ino = dinodeid;
    
    return newinode;
}

void iput(struct inode *pinode) {
    long addr;
    unsigned int block_num;
    int i;
    
    if (pinode->i_count > 1) {
        pinode->i_count--;
        return;
    }
    
    if (pinode->di_number != 0) {
        addr = DINODESTART + pinode->i_ino * DINODESIZ;
        fseek(fd, addr, SEEK_SET);
        fwrite(&(pinode->di_number), DINODESIZ, 1, fd);
    } else {
        block_num = pinode->di_size / BLOCKSIZ;
        for (i = 0; i < block_num; i++) {
            if (pinode->di_addr[i] != 0) {
                bfree(pinode->di_addr[i]);
            }
        }
        ifree(pinode->i_ino);
    }
    
    if (pinode->i_forw != NULL) {
        pinode->i_forw->i_back = pinode->i_back;
    }
    if (pinode->i_back != NULL) {
        pinode->i_back->i_forw = pinode->i_forw;
    } else {
        unsigned int inodeid = pinode->i_ino % NHINO;
        hinode[inodeid].i_forw = pinode->i_forw;
    }
    
    free(pinode);
}

static struct dinode block_buf[BLOCKSIZ / DINODESIZ];

struct inode *ialloc(void) {
    struct inode *temp_inode;
    unsigned int cur_di;
    int i, count, block_end_flag;
    
    if (filsys.s_pinode == NICINOD) {
        i = 0;
        count = 0;
        block_end_flag = 1;
        filsys.s_pinode = NICINOD - 1;
        cur_di = filsys.s_rinode;
        
        while ((count < NICINOD) && (count < filsys.s_ninode)) {
            if (block_end_flag) {
                fseek(fd, DINODESTART + (cur_di / (BLOCKSIZ / DINODESIZ)) * BLOCKSIZ, SEEK_SET);
                fread(block_buf, 1, BLOCKSIZ, fd);
                block_end_flag = 0;
                i = cur_di % (BLOCKSIZ / DINODESIZ);
            }
            
            if (block_buf[i].di_mode == DIEMPTY) {
                filsys.s_inode[filsys.s_pinode--] = cur_di;
                count++;
            }
            
            cur_di++;
            i++;
            if (i >= (BLOCKSIZ / DINODESIZ)) {
                block_end_flag = 1;
            }
        }
        
        filsys.s_rinode = cur_di;
    }
    
    temp_inode = iget(filsys.s_inode[filsys.s_pinode]);
    fseek(fd, DINODESTART + filsys.s_inode[filsys.s_pinode] * DINODESIZ, SEEK_SET);
    fwrite(&(temp_inode->di_number), 1, sizeof(struct dinode), fd);
    
    filsys.s_pinode++;
    filsys.s_ninode--;
    filsys.s_fmod = SUPDATE;
    
    return temp_inode;
}

void ifree(unsigned int dinodeid) {
    filsys.s_ninode++;
    
    if (filsys.s_pinode != NICINOD) {
        filsys.s_inode[filsys.s_pinode] = dinodeid;
        filsys.s_pinode++;
    } else {
        if (dinodeid < filsys.s_rinode) {
            filsys.s_inode[NICINOD] = dinodeid;
            filsys.s_rinode = dinodeid;
        }
    }
}
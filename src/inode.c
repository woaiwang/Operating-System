#include "filesys.h"
#include <stdlib.h>
#include <stdio.h>

/*
 * iget: get an inode by its disk inode number.
 *
 * Ported from orig/igetput.c.  First checks the hash table (hinode[]).
 * If not found, reads the dinode from disk and allocates an in-memory inode.
 *
 * Original bug fix: the original dereferenced newinode->i_forw->i_back
 * before checking whether i_forw was NULL (empty hash chain), causing
 * a null-pointer crash.  We check for NULL first.
 */
struct inode *iget(unsigned int dinodeid) {
    int inodeid;
    struct inode *temp;
    struct inode *newinode;

    inodeid = (int)(dinodeid % NHINO);

    /* search the hash chain */
    if (hinode[inodeid].i_forw != NULL) {
        temp = hinode[inodeid].i_forw;
        while (temp) {
            if (temp->i_ino == dinodeid) {
                temp->i_count++;
                return temp;
            }
            temp = temp->i_forw;
        }
    }

    /* not found — load from disk */
    debug_log("iget: loading dinodeid=%u from disk\n", dinodeid);

    newinode = (struct inode *)malloc(sizeof(struct inode));
    if (!newinode) {
        debug_log("iget: malloc failed\n");
        return NULL;
    }

    /* read dinode from disk */
    if (fseek(fd, (long)DINODESTART + (long)dinodeid * DINODESIZ, SEEK_SET) != 0) {
        debug_log("iget: fseek failed for dinodeid=%u\n", dinodeid);
        free(newinode);
        return NULL;
    }
    if (fread(&(newinode->di_number), 1, sizeof(struct dinode), fd)
        != sizeof(struct dinode)) {
        debug_log("iget: fread failed for dinodeid=%u\n", dinodeid);
        free(newinode);
        return NULL;
    }

    /* insert at head of hash chain */
    newinode->i_forw = hinode[inodeid].i_forw;
    newinode->i_back = (struct inode *)&hinode[inodeid];
    if (hinode[inodeid].i_forw)
        hinode[inodeid].i_forw->i_back = newinode;
    hinode[inodeid].i_forw = newinode;

    newinode->i_count = 1;
    newinode->i_flag  = 0;
    newinode->i_ino   = dinodeid;

    debug_log("iget: loaded dinodeid=%u, di_number=%u, di_mode=%o\n",
              dinodeid, newinode->di_number, newinode->di_mode);
    return newinode;
}

/*
 * iput: release an inode.
 *
 * Ported from orig/igetput.c.  Decrements the reference count.
 * When count reaches 0:
 *  - If di_number != 0, the dinode is written back to disk.
 *  - If di_number == 0, the file's blocks are freed and the inode is returned.
 *
 * Original bug fix: the original called balloc() to free file blocks,
 * but balloc() allocates blocks, not frees them.  Changed to bfree().
 */
void iput(struct inode *pinode) {
    long addr;
    unsigned int block_num;
    int i;

    if (!pinode) return;

    if (pinode->i_count > 1) {
        pinode->i_count--;
        return;
    }

    /* i_count == 1: last reference */
    if (pinode->di_number != 0) {
        /* write back dinode to disk */
        addr = (long)DINODESTART + (long)pinode->i_ino * DINODESIZ;
        if (fseek(fd, addr, SEEK_SET) == 0) {
            fwrite(&pinode->di_number, 1, sizeof(struct dinode), fd);
        }
    } else {
        /* di_number == 0: file is deleted — free its blocks and inode */
        block_num = pinode->di_size / BLOCKSIZ;
        for (i = 0; i < (int)block_num; i++)
            bfree(pinode->di_addr[i]);
        ifree(pinode->i_ino);
    }

    /* remove from hash chain */
    if (pinode->i_forw)
        pinode->i_forw->i_back = pinode->i_back;
    if (pinode->i_back)
        pinode->i_back->i_forw = pinode->i_forw;

    free(pinode);
}

/*
 * ialloc: allocate a free disk inode.
 *
 * Ported from orig/iallfre.c.  The superblock maintains an inode free list
 * cache (s_inode[] / s_pinode).  When the cache is empty, it scans the disk
 * inode table for free entries and refills.
 *
 * Original bug fixes:
 *  - while-condition changed from || to && (the original "||" would
 *    scan past the end of the inode table when s_ninode was small).
 *  - Added NULL return when no inodes are available.
 */
struct inode *ialloc(void) {
    struct inode *temp_inode;
    unsigned int cur_di;
    int i, count, block_end_flag;

    /* static buffer for reading a block of dinodes from disk */
    static struct dinode dinode_buf[BLOCKSIZ / DINODESIZ];

    if (filsys.s_pinode == NICINOD) {
        /* inode cache is empty — refill from disk */
        i = 0;
        count = 0;
        block_end_flag = 1;
        filsys.s_pinode = NICINOD - 1;
        cur_di = filsys.s_rinode;

        while (count < NICINOD && count <= (int)filsys.s_ninode) {
            if (block_end_flag) {
                if (fseek(fd, (long)DINODESTART + (long)cur_di * DINODESIZ,
                          SEEK_SET) != 0) {
                    debug_log("ialloc: fseek failed at cur_di=%u\n", cur_di);
                    return NULL;
                }
                if (fread(dinode_buf, 1, BLOCKSIZ, fd) != BLOCKSIZ) {
                    debug_log("ialloc: fread failed at cur_di=%u\n", cur_di);
                    return NULL;
                }
                block_end_flag = 0;
                i = 0;
            }

            while (i < (int)(BLOCKSIZ / DINODESIZ) &&
                   dinode_buf[i].di_mode == DIEMPTY) {
                cur_di++;
                i++;
            }

            if (i >= (int)(BLOCKSIZ / DINODESIZ)) {
                block_end_flag = 1;
            } else {
                filsys.s_inode[filsys.s_pinode--] = cur_di;
                count++;
            }
        }
        filsys.s_rinode = cur_di;
    }

    /* s_pinode now points to the last valid entry + 1 (i.e. next to take) */
    if (filsys.s_pinode >= NICINOD) {
        /* no free inodes */
        debug_log("ialloc: no free inodes (s_ninode=%u)\n", filsys.s_ninode);
        return NULL;
    }

    temp_inode = iget(filsys.s_inode[filsys.s_pinode]);
    if (!temp_inode) {
        debug_log("ialloc: iget(%u) failed\n", filsys.s_inode[filsys.s_pinode]);
        return NULL;
    }

    /* write the dinode to disk to mark it as allocated */
    if (fseek(fd, (long)DINODESTART +
              (long)filsys.s_inode[filsys.s_pinode] * DINODESIZ, SEEK_SET) != 0) {
        debug_log("ialloc: fseek to dinode failed\n");
        return NULL;
    }
    if (fwrite(&temp_inode->di_number, 1, sizeof(struct dinode), fd)
        != sizeof(struct dinode)) {
        debug_log("ialloc: fwrite dinode failed\n");
        return NULL;
    }

    filsys.s_pinode++;
    filsys.s_ninode--;
    filsys.s_fmod = SUPDATE;
    return temp_inode;
}

/*
 * ifree: return a disk inode to the free list.
 *
 * Ported from orig/iallfre.c.
 *
 * Original bug fix: the original wrote to s_inode[NICINOD] which is
 * out of bounds (valid indices: 0..NICINOD-1).  We guard against that
 * and instead just update s_rinode when the cache is full and the
 * freed inode has a lower number.
 */
void ifree(unsigned int dinodeid) {
    filsys.s_ninode++;

    if (filsys.s_pinode < NICINOD) {
        /* cache has room — add to it */
        if (filsys.s_pinode > 0) {
            filsys.s_pinode--;
            filsys.s_inode[filsys.s_pinode] = dinodeid;
        } else {
            /* s_pinode == 0: cache is completely full (all slots used) */
            if (dinodeid < filsys.s_rinode)
                filsys.s_rinode = dinodeid;
        }
    } else {
        /* s_pinode >= NICINOD: no room in cache, just update rinode */
        if (dinodeid < filsys.s_rinode)
            filsys.s_rinode = dinodeid;
    }
}

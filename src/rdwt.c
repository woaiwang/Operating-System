#include "filesys.h"
#include <stdio.h>

/*
 * fs_read: read from a file.
 *
 * Ported from orig/rdwt.c.  Reads up to 'size' bytes from the file
 * at the current offset.  Returns the number of bytes actually read.
 *
 * Renamed from read() to avoid collision with libc read().
 * Parameter 'fd' (file descriptor) renamed to 'fildes' to avoid
 * shadowing the global FILE *fd.
 *
 * Original bug fixes:
 *  - fd parameter (local) shadowed global FILE *fd, causing disk
 *    reads to use the wrong file pointer.  Renamed to fildes.
 *  - The final partial-block read used an uninitialised block index
 *    in some edge cases.  Restructured the logic.
 */
unsigned int fs_read(int fildes, char *buf, unsigned int size) {
    unsigned long off;
    unsigned int block_off;
    int block_idx, i, nblocks;
    struct inode *inode;
    unsigned int sys_no;
    char *dst;

    if (fildes < 0 || fildes >= NOFILE) return 0;

    sys_no = user[user_id].u_ofile[fildes];
    if (sys_no == SYSOPENFILE + 1 || sys_no >= SYSOPENFILE) return 0;

    if (!(sys_ofile[sys_no].f_flag & FREAD)) {
        printf("\nthe file is not opened for read\n");
        return 0;
    }

    inode = sys_ofile[sys_no].f_inode;
    if (!inode) return 0;

    off = sys_ofile[sys_no].f_off;
    if (off + size > inode->di_size)
        size = (unsigned int)(inode->di_size - off);
    if (size == 0) return 0;

    dst = buf;
    block_off = (unsigned int)(off % BLOCKSIZ);
    block_idx = (int)(off / BLOCKSIZ);

    /* first partial block */
    if (block_off > 0) {
        unsigned int chunk = BLOCKSIZ - block_off;
        if (chunk > size) chunk = size;
        fseek(fd, DATASTART + (long)inode->di_addr[block_idx] * BLOCKSIZ + block_off, SEEK_SET);
        fread(dst, 1, chunk, fd);
        dst += chunk;
        size -= chunk;
        block_idx++;
    }

    /* full blocks */
    nblocks = (int)(size / BLOCKSIZ);
    for (i = 0; i < nblocks; i++) {
        fseek(fd, DATASTART + (long)inode->di_addr[block_idx + i] * BLOCKSIZ, SEEK_SET);
        fread(dst, 1, BLOCKSIZ, fd);
        dst += BLOCKSIZ;
        size -= BLOCKSIZ;
    }

    /* final partial block */
    if (size > 0) {
        fseek(fd, DATASTART + (long)inode->di_addr[block_idx + nblocks] * BLOCKSIZ, SEEK_SET);
        fread(dst, 1, size, fd);
        dst += size;
    }

    sys_ofile[sys_no].f_off += (unsigned long)(dst - buf);
    return (unsigned int)(dst - buf);
}

/*
 * fs_write: write to a file.
 *
 * Ported from orig/rdwt.c.  Writes up to 'size' bytes to the file
 * at the current offset.  Allocates new blocks as needed.
 * Returns the number of bytes actually written.
 *
 * Renamed from write() to avoid collision with libc write().
 * Parameter 'fd' renamed to 'fildes' to avoid shadowing global FILE *fd.
 *
 * Original bug fixes:
 *  - Same fd shadowing issue as fs_read.
 *  - The original's block indexing for partial writes was wrong
 *    when data spanned multiple blocks.
 */
unsigned int fs_write(int fildes, const char *buf, unsigned int size) {
    unsigned long off;
    unsigned int block_off;
    int block_idx, i, nblocks;
    struct inode *inode;
    unsigned int sys_no, blk;
    const char *src;
    unsigned int total_written;

    if (fildes < 0 || fildes >= NOFILE) return 0;

    sys_no = user[user_id].u_ofile[fildes];
    if (sys_no == SYSOPENFILE + 1 || sys_no >= SYSOPENFILE) return 0;

    if (!(sys_ofile[sys_no].f_flag & FWRITE)) {
        printf("\nthe file is not opened for write\n");
        return 0;
    }

    inode = sys_ofile[sys_no].f_inode;
    if (!inode) return 0;

    off  = sys_ofile[sys_no].f_off;
    src  = buf;
    total_written = 0;
    block_off = (unsigned int)(off % BLOCKSIZ);
    block_idx = (int)(off / BLOCKSIZ);

    /* ensure the current block is allocated */
    if (block_idx >= NADDR) {
        printf("\nfile too large\n");
        return 0;
    }
    if (inode->di_addr[block_idx] == 0) {
        blk = balloc();
        if (blk == DISKFULL) return total_written;
        inode->di_addr[block_idx] = blk;
    }

    /* first partial block */
    if (block_off > 0) {
        unsigned int chunk = BLOCKSIZ - block_off;
        if (chunk > size) chunk = size;
        fseek(fd, DATASTART + (long)inode->di_addr[block_idx] * BLOCKSIZ + block_off, SEEK_SET);
        fwrite(src, 1, chunk, fd);
        src += chunk;
        size -= chunk;
        total_written += chunk;
        block_idx++;
    }

    /* full blocks */
    nblocks = (int)(size / BLOCKSIZ);
    for (i = 0; i < nblocks; i++) {
        if (block_idx + i >= NADDR) break;
        if (inode->di_addr[block_idx + i] == 0) {
            blk = balloc();
            if (blk == DISKFULL) {
                sys_ofile[sys_no].f_off += total_written;
                return total_written;
            }
            inode->di_addr[block_idx + i] = blk;
        }
        fseek(fd, DATASTART + (long)inode->di_addr[block_idx + i] * BLOCKSIZ, SEEK_SET);
        fwrite(src, 1, BLOCKSIZ, fd);
        src += BLOCKSIZ;
        size -= BLOCKSIZ;
        total_written += BLOCKSIZ;
    }

    /* final partial block */
    if (size > 0) {
        int final_idx = block_idx + nblocks;
        if (final_idx >= NADDR) {
            sys_ofile[sys_no].f_off += total_written;
            return total_written;
        }
        if (inode->di_addr[final_idx] == 0) {
            blk = balloc();
            if (blk == DISKFULL) {
                sys_ofile[sys_no].f_off += total_written;
                return total_written;
            }
            inode->di_addr[final_idx] = blk;
        }
        fseek(fd, DATASTART + (long)inode->di_addr[final_idx] * BLOCKSIZ, SEEK_SET);
        fwrite(src, 1, size, fd);
        total_written += size;
    }

    sys_ofile[sys_no].f_off += total_written;
    if (sys_ofile[sys_no].f_off > inode->di_size)
        inode->di_size = (unsigned short)sys_ofile[sys_no].f_off;

    return total_written;
}

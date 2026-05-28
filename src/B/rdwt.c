#include "filesys.h"
#include <stdio.h>

/*
 * fs_read: 从文件中读取数据
 *
 * 从当前偏移量开始，最多读取 size 字节到 buf 中
 * 返回实际读取的字节数
 *
 * 命名说明：
 *  - 函数名从 read 改为 fs_read，避免与 libc 的 read() 冲突
 *  - 参数名从 fd 改为 fildes，避免与全局 FILE *fd 冲突
 *
 * 原始 bug 修复：
 *  - fd 参数遮蔽了全局 FILE *fd，导致磁盘读写使用错误的文件指针
 *    已重命名为 fildes
 *  - 某些边界情况下最后一个非完整块使用了未初始化的块索引
 *    已重构逻辑
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
        printf("\n文件未以读模式打开\n");
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

    /* 第一个非完整块 */
    if (block_off > 0) {
        unsigned int chunk = BLOCKSIZ - block_off;
        if (chunk > size) chunk = size;
        /* 检查块是否已分配 */
        if (inode->di_addr[block_idx] == 0) return (unsigned int)(dst - buf);
        fseek(fd, DATASTART + (long)inode->di_addr[block_idx] * BLOCKSIZ + block_off, SEEK_SET);
        fread(dst, 1, chunk, fd);
        dst += chunk;
        size -= chunk;
        block_idx++;
    }

    /* 完整块 */
    nblocks = (int)(size / BLOCKSIZ);
    for (i = 0; i < nblocks; i++) {
        if (inode->di_addr[block_idx + i] == 0) break;
        fseek(fd, DATASTART + (long)inode->di_addr[block_idx + i] * BLOCKSIZ, SEEK_SET);
        fread(dst, 1, BLOCKSIZ, fd);
        dst += BLOCKSIZ;
        size -= BLOCKSIZ;
    }

    /* 最后一个非完整块 */
    if (size > 0) {
        if (inode->di_addr[block_idx + nblocks] == 0)
            return (unsigned int)(dst - buf);
        fseek(fd, DATASTART + (long)inode->di_addr[block_idx + nblocks] * BLOCKSIZ, SEEK_SET);
        fread(dst, 1, size, fd);
        dst += size;
    }

    sys_ofile[sys_no].f_off += (unsigned long)(dst - buf);
    return (unsigned int)(dst - buf);
}

/*
 * fs_write: 向文件中写入数据
 *
 * 从当前偏移量开始，写入最多 size 字节
 * 按需分配新数据块，返回实际写入的字节数
 *
 * 命名说明：
 *  - 函数名从 write 改为 fs_write，避免与 libc 的 write() 冲突
 *  - 参数名从 fd 改为 fildes，避免与全局 FILE *fd 冲突
 *
 * 原始 bug 修复：
 *  - 与 fs_read 相同的 fd 遮蔽问题
 *  - 原来跨多块的部分写入时块索引计算有误，已修复
 *  - 增加对未分配块的检查，防止写入到无效位置
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
        printf("\n文件未以写模式打开\n");
        return 0;
    }

    inode = sys_ofile[sys_no].f_inode;
    if (!inode) return 0;

    off  = sys_ofile[sys_no].f_off;
    src  = buf;
    total_written = 0;
    block_off = (unsigned int)(off % BLOCKSIZ);
    block_idx = (int)(off / BLOCKSIZ);

    /* 确保当前块已分配 */
    if (block_idx >= NADDR) {
        printf("\n文件过大\n");
        return 0;
    }
    if (inode->di_addr[block_idx] == 0) {
        blk = balloc();
        if (blk == DISKFULL) return total_written;
        inode->di_addr[block_idx] = blk;
    }

    /* 第一个非完整块 */
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

    /* 完整块 */
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

    /* 最后一个非完整块 */
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

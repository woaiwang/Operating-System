#include "filesys.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Free block management — Unix V6 style linked free-block chain.
 *
 * s_free[0..NICFREE-1] caches free data-block indices.
 * s_pfree = count of valid entries in s_free[] (stack height).
 *   - s_pfree == 0        : cache empty (needs refill from disk chain)
 *   - s_pfree == NICFREE  : cache full (needs spill before next bfree)
 *
 * Refill:  read a chain block from disk into s_free[].
 * Spill:   write current s_free[] into the block being freed, turning
 *          that data block into a chain-link block.
 */

/* buffer for disk chain block r/w: NICFREE block numbers + 1 next-pointer */
typedef unsigned int chain_block_t[NICFREE + 1];

/* head of the on-disk free-block chain; updated by both balloc and bfree */
static unsigned int chain_head = FILEBLK + 1;

/*
 * Refill the cache from a disk chain block.
 * next_blk is the data-block index of the chain block to load.
 * On return, s_pfree is set to the number of blocks loaded (<= NICFREE),
 * and *next_blk is updated to the next chain block (or FILEBLK+1 sentinel).
 */
static void refill_from_chain(unsigned int *next_blk) {
    chain_block_t cb;
    unsigned int nblocks, i;

    if (*next_blk >= (unsigned int)FILEBLK + 1) {
        /* sentinel: no more chain blocks — disk truly full */
        debug_log("refill_from_chain: hit sentinel (%u)\n", *next_blk);
        return;
    }

    /* read the chain block from the data area */
    if (fseek(fd, (long)DATASTART + (long)(*next_blk) * BLOCKSIZ, SEEK_SET) != 0) {
        debug_log("refill_from_chain: fseek to blk %u failed\n", *next_blk);
        exit(1);
    }
    if (fread(cb, 1, BLOCKSIZ, fd) != BLOCKSIZ) {
        debug_log("refill_from_chain: fread blk %u failed\n", *next_blk);
        exit(1);
    }

    /* cb[0..NICFREE-1] = free block numbers, cb[NICFREE] = next chain blk */
    nblocks = NICFREE;
    for (i = 0; i < nblocks; i++)
        filsys.s_free[i] = cb[i];
    filsys.s_pfree = (unsigned short)nblocks;
    *next_blk = cb[NICFREE];

    debug_log("refill_from_chain: loaded %u blocks, next_chain=%u, s_nfree=%u\n",
              nblocks, *next_blk, filsys.s_nfree);
}

/*
 * Spill the current cache into the given data block, turning it
 * into a chain-link block on disk.
 */
static void spill_to_chain(unsigned int spill_blk) {
    chain_block_t cb;
    unsigned int i;

    /*
     * The chain block's next-pointer is set to FILEBLK+1 (sentinel).
     * A full chain implementation would chain through previously
     * spilled blocks.  For our initial implementation, each spill
     * is a standalone chain node — the last-spilled block is the
     * first one read back on refill.
     */
    for (i = 0; i < filsys.s_pfree; i++)
        cb[i] = filsys.s_free[i];
    cb[NICFREE] = FILEBLK + 1;  /* sentinel: end of chain */

    if (fseek(fd, (long)DATASTART + (long)spill_blk * BLOCKSIZ, SEEK_SET) != 0) {
        debug_log("spill_to_chain: fseek to blk %u failed\n", spill_blk);
        exit(1);
    }
    if (fwrite(cb, 1, BLOCKSIZ, fd) != BLOCKSIZ) {
        debug_log("spill_to_chain: fwrite blk %u failed\n", spill_blk);
        exit(1);
    }

    debug_log("spill_to_chain: spilled %u blocks to data blk %u\n",
              (unsigned)filsys.s_pfree, spill_blk);

    filsys.s_pfree = 0;  /* cache is now empty */
}

unsigned int balloc(void) {
    unsigned int free_block;

    if (filsys.s_nfree == 0) {
        printf("Disk Full!!!\n");
        return DISKFULL;
    }

    if (filsys.s_pfree == 0) {
        /*
         * Cache empty — refill from the disk chain.
         */
        if (chain_head >= (unsigned int)FILEBLK + 1) {
            /* No chain blocks on disk.  After format this should not
             * happen because bfree() spills create chain blocks. */
            chain_head = FILEBLK - NICFREE - 1;
        }

        refill_from_chain(&chain_head);
    }

    if (filsys.s_pfree == 0) {
        /* still empty after refill — no free blocks */
        printf("Disk Full!!!\n");
        return DISKFULL;
    }

    filsys.s_pfree--;
    free_block = filsys.s_free[filsys.s_pfree];
    filsys.s_nfree--;
    filsys.s_fmod = SUPDATE;
    return free_block;
}

void bfree(unsigned int block_num) {
    if (filsys.s_pfree >= NICFREE) {
        /* cache full — spill it to the block being freed */
        chain_head = block_num;  /* this block becomes a chain node on disk */
        spill_to_chain(block_num);
        /* after spill, s_pfree == 0 */
    }

    /* push block_num onto the stack */
    filsys.s_free[filsys.s_pfree] = block_num;
    filsys.s_pfree++;
    filsys.s_nfree++;
    filsys.s_fmod = SUPDATE;
}

void bread(unsigned int blkno, char *buf) {
    size_t n;
    if (!fd || !buf) {
        debug_log("bread: bad args (fd=%p buf=%p)\n", (void *)fd, (void *)buf);
        return;
    }
    if (fseek(fd, (long)blkno * BLOCKSIZ, SEEK_SET) != 0) {
        debug_log("bread: fseek blkno=%u failed\n", blkno);
        return;
    }
    n = fread(buf, 1, BLOCKSIZ, fd);
    if (n != BLOCKSIZ)
        debug_log("bread: fread blkno=%u short (%zu/%d)\n", blkno, n, BLOCKSIZ);
}

void bwrite(unsigned int blkno, const char *buf) {
    size_t n;
    if (!fd || !buf) {
        debug_log("bwrite: bad args (fd=%p buf=%p)\n", (void *)fd, (void *)buf);
        return;
    }
    if (fseek(fd, (long)blkno * BLOCKSIZ, SEEK_SET) != 0) {
        debug_log("bwrite: fseek blkno=%u failed\n", blkno);
        return;
    }
    n = fwrite(buf, 1, BLOCKSIZ, fd);
    if (n != BLOCKSIZ)
        debug_log("bwrite: fwrite blkno=%u short (%zu/%d)\n", blkno, n, BLOCKSIZ);
}

#include "filesys.h"
#include <string.h>

unsigned int balloc(void) {
    /* TODO: implement */
    return 0;
}

void bfree(unsigned int blkno) {
    (void)blkno;
    /* TODO: implement */
}

void bread(unsigned int blkno, char *buf) {
    (void)blkno;
    if (buf) memset(buf, 0, BLKSIZE);
    /* TODO: implement */
}

void bwrite(unsigned int blkno, const char *buf) {
    (void)blkno;
    (void)buf;
    /* TODO: implement */
}
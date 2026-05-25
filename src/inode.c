#include "filesys.h"

struct inode *iget(unsigned int ino) {
    (void)ino;
    /* TODO: implement */
    return 0;
}

void iput(struct inode *ip) {
    (void)ip;
    /* TODO: implement */
}

unsigned int ialloc(void) {
    /* TODO: implement */
    return 0;
}

void ifree(unsigned int ino) {
    (void)ino;
    /* TODO: implement */
}
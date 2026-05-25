#include "filesys.h"
#include <stdio.h>

int main(void) {
    puts("smoke test start");

    install();
    format();

    unsigned int b = balloc();
    printf("balloc returned: %u\n", b);
    bfree(b);

    unsigned int ino = ialloc();
    printf("ialloc returned: %u\n", ino);
    ifree(ino);

    halt();
    puts("smoke test end");
    return 0;
}
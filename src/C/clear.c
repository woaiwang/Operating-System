#include "filesys.h"
#include <stdio.h>

void clear(void) {
    printf("\033[H\033[J");
    fflush(stdout);
}

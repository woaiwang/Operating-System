#include "filesys.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--test") == 0) {
        return run_all_tests();
    }
    printf("Type 'format' to create a new filesystem, or 'install' to mount.\n");
    printf("Type 'help' for commands.\n\n");
    return file_main();
}

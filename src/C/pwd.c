#include "filesys.h"
#include <stdio.h>
#include <string.h>

static char current_path[512] = "/";

void set_current_path(const char *path) {
    strncpy(current_path, path, sizeof(current_path) - 1);
    current_path[sizeof(current_path) - 1] = '\0';
}

const char *get_current_path(void) {
    return current_path;
}

void my_pwd(void) {
    printf("%s\n", current_path);
}

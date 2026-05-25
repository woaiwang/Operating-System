#include "filesys.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * file.c: interactive file system shell.
 *
 * Ported from orig/FILE.C.  The original was a standalone test program
 * using libc fopen/fputc/fgetc.  We reimplement it as an interactive
 * shell over our own filesystem API: format/install, file/dir ops,
 * and halt.
 */

static void show_help(void) {
    printf("\nCommands:\n");
    printf("  help                  - show this help\n");
    printf("  format                - create a new filesystem image\n");
    printf("  install               - mount existing filesystem image\n");
    printf("  halt                  - unmount and exit\n");
    printf("  dir                   - list current directory\n");
    printf("  mkdir <name>          - create a directory\n");
    printf("  chdir <name>          - change directory\n");
    printf("  creat <name>          - create a file\n");
    printf("  open <name> [r|w|a]   - open a file\n");
    printf("  close <fd>            - close a file descriptor\n");
    printf("  read  <fd> <nbytes>   - read from a file\n");
    printf("  write <fd> <text>     - write text to a file\n");
    printf("  delete <name>         - delete a file\n");
    printf("  login <uid> <pwd>     - log in (simplified)\n");
    printf("  logout                - log out\n");
    printf("\n");
}

int file_main(void) {
    char cmd[64];
    char arg1[128];
    char arg2[256];
    int running = 1;
    int need_init = 1;
    int n;

    printf("=== Filesys Shell ===\n");
    printf("Type 'help' for commands.\n");

    while (running) {
        printf("\nfs> ");
        fflush(stdout);

        if (!fgets(cmd, sizeof(cmd), stdin)) break;

        /* strip newline */
        cmd[strcspn(cmd, "\n")] = '\0';
        if (cmd[0] == '\0') continue;

        /* parse command */
        arg1[0] = '\0';
        arg2[0] = '\0';
        sscanf(cmd, "%s %s %[^\n]", cmd, arg1, arg2);

        if (strcmp(cmd, "help") == 0) {
            show_help();

        } else if (strcmp(cmd, "format") == 0) {
            format();
            cur_path_inode = iget(1);
            need_init = 0;
            printf("Filesystem formatted.\n");

        } else if (strcmp(cmd, "install") == 0) {
            if (!need_init) halt();
            install();
            cur_path_inode = iget(1);
            if (cur_path_inode)
                dir.size = 0;
            need_init = 0;
            printf("Filesystem mounted.\n");

        } else if (strcmp(cmd, "halt") == 0) {
            halt();
            running = 0;

        } else if (strcmp(cmd, "dir") == 0) {
            if (cur_path_inode)
                _dir();
            else
                printf("No filesystem mounted. Use 'format' or 'install' first.\n");

        } else if (strcmp(cmd, "mkdir") == 0) {
            if (arg1[0])
                mkdir(arg1);
            else
                printf("Usage: mkdir <name>\n");

        } else if (strcmp(cmd, "chdir") == 0) {
            if (arg1[0])
                chdir(arg1);
            else
                printf("Usage: chdir <name>\n");

        } else if (strcmp(cmd, "creat") == 0) {
            if (arg1[0]) {
                int fd = creat(arg1);
                if (fd >= 0) printf("File created, fd=%d\n", fd);
            } else
                printf("Usage: creat <name>\n");

        } else if (strcmp(cmd, "open") == 0) {
            if (arg1[0]) {
                unsigned int mode = FREAD;
                if (arg2[0] == 'w') mode = FWRITE;
                else if (arg2[0] == 'a') mode = FAPPEND | FWRITE;
                unsigned short fd = aopen(arg1, mode);
                if (fd > 0) printf("File opened, fd=%u\n", fd);
            } else
                printf("Usage: open <name> [r|w|a]\n");

        } else if (strcmp(cmd, "close") == 0) {
            if (arg1[0]) {
                int fd = atoi(arg1);
                close(fd);
                printf("Closed fd=%d\n", fd);
            } else
                printf("Usage: close <fd>\n");

        } else if (strcmp(cmd, "read") == 0) {
            if (arg1[0] && arg2[0]) {
                int fd = atoi(arg1);
                int nbytes = atoi(arg2);
                char *buf = (char *)malloc((size_t)nbytes + 1);
                if (buf) {
                    memset(buf, 0, (size_t)nbytes + 1);
                    n = (int)fs_read(fd, buf, (unsigned int)nbytes);
                    printf("Read %d bytes: %.*s\n", n, n, buf);
                    free(buf);
                }
            } else
                printf("Usage: read <fd> <nbytes>\n");

        } else if (strcmp(cmd, "write") == 0) {
            if (arg1[0] && arg2[0]) {
                int fd = atoi(arg1);
                n = (int)fs_write(fd, arg2, (unsigned int)strlen(arg2));
                printf("Wrote %d bytes\n", n);
            } else
                printf("Usage: write <fd> <text>\n");

        } else if (strcmp(cmd, "delete") == 0) {
            if (arg1[0])
                delete(arg1);
            else
                printf("Usage: delete <name>\n");

        } else if (strcmp(cmd, "login") == 0) {
            if (arg1[0] && arg2[0]) {
                if (login(arg1, arg2))
                    printf("Login successful.\n");
            } else
                printf("Usage: login <name> <password>\n");

        } else if (strcmp(cmd, "logout") == 0) {
            logout();
            printf("Logged out.\n");

        } else {
            printf("Unknown command: %s\n", cmd);
        }
    }

    return 0;
}

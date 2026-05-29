#include "filesys.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define DISK_IMAGE "build/filesystem.img"

static void die(const char *msg) {
    printf("install FATAL: %s (errno=%d: %s)\n", msg, errno, strerror(errno));
    fflush(stdout);
    exit(1);
}

void install(const char *image_path) {
    int i, j;

    if (!image_path || !image_path[0])
        image_path = "build/filesystem.img";

    debug_log("install: mounting %s ...\n", image_path);

    fd = fopen(image_path, "r+b");
    if (!fd) die("cannot open disk image");

    /* read superblock from block 1 */
    if (fseek(fd, BLOCKSIZ, SEEK_SET) != 0)
        die("fseek to superblock failed");
    if (fread(&filsys, sizeof(struct filsys), 1, fd) != 1)
        die("fread superblock failed");

    debug_log("install: superblock loaded: isize=%u fsize=%lu ninode=%u nfree=%u\n",
              (unsigned)filsys.s_isize, (unsigned long)filsys.s_fsize,
              filsys.s_ninode, filsys.s_nfree);

    /* initialize inode hash chain */
    for (i = 0; i < NHINO; i++)
        hinode[i].i_forw = NULL;

    /* initialize system open file table */
    for (i = 0; i < SYSOPENFILE; i++) {
        sys_ofile[i].f_count = 0;
        sys_ofile[i].f_inode = NULL;
    }

    /* initialize user table */
    for (i = 0; i < USERNUM; i++) {
        user[i].u_uid  = 0;
        user[i].u_gid  = 0;
        for (j = 0; j < NOFILE; j++)
            user[i].u_ofile[j] = SYSOPENFILE + 1;
    }

    /* root dir inode will be loaded when iget is available */
    cur_path_inode = NULL;
    (void)dir;
    set_current_path("/");

    debug_log("install: done.\n");
}

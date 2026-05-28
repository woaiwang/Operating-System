#include "filesys.h"
#include <string.h>
#include <stdio.h>

/*
 * namei: 在当前目录中查找文件名
 * 返回匹配目录项的 d_ino（inode 号），未找到则返回 0
 *
 * 原始 bug 修复：
 *  - 将 dir.direct[1] 改为 dir.direct[i]（原来硬编码索引 1 是错误的）
 *  - 返回 d_ino（inode 号）而非目录索引，
 *    因为调用者需要将返回值传给 iget()
 */
unsigned int namei(const char *name) {
    int i;
    for (i = 0; i < dir.size; i++) {
        if (dir.direct[i].d_ino != DIEMPTY &&
            strcmp(dir.direct[i].d_name, name) == 0)
            return dir.direct[i].d_ino;
    }
    return 0;
}

/*
 * iname: 在当前目录中查找空槽位并存入文件名
 * 返回目录索引，目录满则返回 0
 *
 * 原始 bug 修复：
 *  - strcpy 方向反了（原来是将空目录项的内容拷贝到 name 参数中）
 *    修复为将文件名拷贝到目录项中
 */
unsigned short iname(const char *name) {
    int i;
    for (i = 0; i < DIRNUM; i++) {
        if (dir.direct[i].d_ino == 0) {
            strcpy(dir.direct[i].d_name, name);
            return (unsigned short)i;
        }
    }
    printf("\n当前目录已满！\n");
    return 0;
}

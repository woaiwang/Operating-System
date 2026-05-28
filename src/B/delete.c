#include "filesys.h"
#include <string.h>
#include <stdio.h>

/*
 * delete: 从当前目录中删除一个文件
 *
 * 清除目录项，然后递减链接计数
 * 当链接计数降为 0 时，iput() 将释放数据块和 inode
 *
 * 原始 bug 修复：
 *  - 增加对 namei 返回值的 NULL 检查
 *  - 增加目录项清理（原代码缺失 — 删除后文件名仍残留在目录中）
 *  - 修复 di_number 递减时的下溢风险
 */
void delete(const char *name) {
    unsigned int dinodeid;
    struct inode *inode;
    int i;

    dinodeid = namei(name);
    if (dinodeid == 0) {
        printf("\ndelete: 文件未找到\n");
        return;
    }

    /* 清除目录项 */
    for (i = 0; i < dir.size; i++) {
        if (dir.direct[i].d_ino == dinodeid &&
            strcmp(dir.direct[i].d_name, name) == 0) {
            memset(&dir.direct[i], 0, sizeof(struct direct));
            break;
        }
    }

    inode = iget(dinodeid);
    if (!inode) return;

    if (inode->di_number > 0)
        inode->di_number--;
    iput(inode);
}

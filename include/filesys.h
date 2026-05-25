#ifndef FILESYS_H
#define FILESYS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ====== 基本常量（先占位，后续根据原始源码调整） ====== */
#define BLKSIZE 512

/* ====== 核心数据结构（先给最小定义，字段后续补齐） ====== */
struct inode {
    uint16_t i_number;     /* inode number */
    uint16_t i_count;      /* reference count */
    /* TODO: add mode/size/addr/etc based on original FILESYS.H */
};

/* ====== A 层对外接口（你之前约定的那批） ====== */
unsigned int balloc(void);
void bfree(unsigned int blkno);
void bread(unsigned int blkno, char *buf);
void bwrite(unsigned int blkno, const char *buf);

struct inode *iget(unsigned int ino);
void iput(struct inode *ip);

unsigned int ialloc(void);
void ifree(unsigned int ino);

void format(void);
void install(void);
void halt(void);

#ifdef __cplusplus
}
#endif

#endif /* FILESYS_H */
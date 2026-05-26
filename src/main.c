#include <stdio.h>
#include "filesys.h"
#include <stdlib.h>
#include <conio.h>
#include <string.h>

struct hinode hinode[NHINO];
struct dir dir;
struct file sys_ofile[SYSOPENFILE];
struct filsys filsys;
struct pwd pwd[PWDNUM];
struct user user[USERNUM];
FILE *fd;
struct inode *cur_path_inode;
int user_id = -1;

void display_help() {
    printf("\n=== FAT文件系统命令列表 ===\n");
    printf("1.  login <uid> <password>  - 用户登录\n");
    printf("2.  logout                  - 用户登出\n");
    printf("3.  dir                     - 显示当前目录内容\n");
    printf("4.  mkdir <dirname>         - 创建目录\n");
    printf("5.  chdir <dirname>         - 切换目录\n");
    printf("6.  creat <filename> <mode> - 创建文件\n");
    printf("7.  delete <filename>       - 删除文件\n");
    printf("8.  open <filename> <mode>  - 打开文件\n");
    printf("9.  close <fd>              - 关闭文件\n");
    printf("10. read <fd> <size>        - 读取文件\n");
    printf("11. write <fd> <size>       - 写入文件\n");
    printf("12. format                  - 格式化磁盘\n");
    printf("13. halt                    - 退出系统\n");
    printf("14. help                    - 显示帮助\n");
    printf("15. exit                    - 退出程序\n");
    printf("==============================\n");
}

void execute_command(char *cmd) {
    char command[100];
    char arg1[50], arg2[50], arg3[50];
    int fd_num, size;
    unsigned short mode;
    
    if (sscanf(cmd, "%s", command) != 1) return;
    
    if (strcmp(command, "login") == 0) {
        if (sscanf(cmd, "%s %hu %s", command, &mode, arg2) == 3) {
            if (login(mode, arg2)) {
                printf("登录成功！用户ID: %hu\n", mode);
                user_id = 0;
            } else {
                printf("登录失败！\n");
            }
        } else {
            printf("用法: login <uid> <password>\n");
        }
    }
    else if (strcmp(command, "logout") == 0) {
        if (user_id >= 0 && user[user_id].u_uid != 0) {
            logout(user[user_id].u_uid);
            printf("用户已登出\n");
            user_id = -1;
        } else {
            printf("没有用户登录\n");
        }
    }
    else if (strcmp(command, "dir") == 0) {
        _dir();
    }
    else if (strcmp(command, "mkdir") == 0) {
        if (sscanf(cmd, "%s %s", command, arg1) == 2) {
            mkdir(arg1);
            printf("目录 %s 创建成功\n", arg1);
        } else {
            printf("用法: mkdir <dirname>\n");
        }
    }
    else if (strcmp(command, "chdir") == 0) {
        if (sscanf(cmd, "%s %s", command, arg1) == 2) {
            chdir(arg1);
        } else {
            printf("用法: chdir <dirname>\n");
        }
    }
    else if (strcmp(command, "creat") == 0) {
        if (sscanf(cmd, "%s %s %ho", command, arg1, &mode) == 3) {
            unsigned short result = creat(user_id, arg1, mode);
            if (result != (unsigned short)-1) {
                printf("文件 %s 创建成功，文件描述符: %hu\n", arg1, result);
            }
        } else {
            printf("用法: creat <filename> <mode>\n");
        }
    }
    else if (strcmp(command, "delete") == 0) {
        if (sscanf(cmd, "%s %s", command, arg1) == 2) {
            delete(arg1);
        } else {
            printf("用法: delete <filename>\n");
        }
    }
    else if (strcmp(command, "open") == 0) {
        if (sscanf(cmd, "%s %s %ho", command, arg1, &mode) == 3) {
            unsigned short result = aopen(user_id, arg1, mode);
            if (result != 0) {
                printf("文件 %s 打开成功，文件描述符: %hu\n", arg1, result);
            }
        } else {
            printf("用法: open <filename> <mode>\n");
        }
    }
    else if (strcmp(command, "close") == 0) {
        if (sscanf(cmd, "%s %hu", command, &fd_num) == 2) {
            close(user_id, fd_num);
            printf("文件描述符 %hu 已关闭\n", fd_num);
        } else {
            printf("用法: close <fd>\n");
        }
    }
    else if (strcmp(command, "read") == 0) {
        if (sscanf(cmd, "%s %hu %d", command, &fd_num, &size) == 3) {
            char *buf = (char *)malloc(size);
            if (buf) {
                unsigned int result = read(fd_num, buf, size);
                printf("读取了 %u 字节\n", result);
                free(buf);
            }
        } else {
            printf("用法: read <fd> <size>\n");
        }
    }
    else if (strcmp(command, "write") == 0) {
        if (sscanf(cmd, "%s %hu %d", command, &fd_num, &size) == 3) {
            char *buf = (char *)malloc(size);
            if (buf) {
                memset(buf, 'A', size);
                unsigned int result = write(fd_num, buf, size);
                printf("写入了 %u 字节\n", result);
                free(buf);
            }
        } else {
            printf("用法: write <fd> <size>\n");
        }
    }
    else if (strcmp(command, "format") == 0) {
        printf("\n格式化将擦除磁盘上的所有内容！\n");
        printf("确定要格式化吗？(y/n): ");
        if (getch() == 'y') {
            printf("\n正在格式化...\n");
            format();
            printf("格式化完成！\n");
        } else {
            printf("取消格式化\n");
        }
    }
    else if (strcmp(command, "halt") == 0) {
        printf("正在关闭系统...\n");
        halt();
    }
    else if (strcmp(command, "help") == 0) {
        display_help();
    }
    else if (strcmp(command, "exit") == 0) {
        printf("退出程序\n");
        exit(0);
    }
    else {
        printf("未知命令: %s，输入 help 查看可用命令\n", command);
    }
}

void main() {
    char input[256];
    
    printf("=== FAT文件系统模拟程序 ===\n");
    printf("输入 'help' 查看可用命令\n");
    printf("输入 'exit' 退出程序\n\n");
    
    install();
    
    while (1) {
        printf("filesys> ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;
        
        if (strlen(input) > 0) {
            execute_command(input);
        }
    }
}
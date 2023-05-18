#include <string.h>
#include <stdio.h>

#include "file_ops.h"
#include "block_layer.h"

#define SUPER_BLOCK 0
#define DATA_BEGIN_BLOCK 20

// 工具函数
int str_to_int(char* buf, int len) {
    int num = 0;
    for (int i=0; i<len; ++i) {
        num = num * 10 + buf[i] - '0';
    }
    return num;
}

void int_to_str(int n, char *str){
    char buf[256];
    int i = 0, tmp = n;

    if(!str){
        return;
    }
    while(tmp){
        buf[i] = (char)(tmp % 10) + '0';
	tmp /= 10;
	i++;
    }
    int len = i;
    str[i] = '\0';
    while(i > 0){
	str[len - i] = buf[i - 1];
        i--;
    }
}


// 按照文件名查找对应的inode
int get_inode_by_filename(const char *name) {
    char buffer[BLOCK_SIZE];
    sd_bread(SUPER_BLOCK, buffer);
    int buffer_size = strlen(buffer);
    //printf("[get_inode_by_filename]read buffer size: %d and buffer: %s\n", buffer_size, buffer);
    // 匹配文件名,目录格式:filename-inodeNum;filename-inodeNum;……
    int fileName_base = 0;
    int inode_base = 0;
    int find = 0;
    for (int i=0; i<buffer_size; ++i) {
        // 找到一个filename-inode映射，看看文件名是否是要找的那个
        if (buffer[i]=='-') {
            int j=fileName_base;
            for (; j<i; ++j) {
                if (buffer[j] != name[j-fileName_base]) {
                    break;
                }
            }
            // 如果是要找的文件，获取inodeNum后返回
            if (j == i) {
                find = 1;
            }
            // 维护inode_base的起始地址
            inode_base = i+1;
        }
        // 找到一个filename-inode映射，看看是否需要获取inodeNum
        if (buffer[i]==';') {
            // 获取inodeNum返回
            if (find == 1) {
                int len = i - inode_base;
                char inodeNumStr[128];
                strncpy(inodeNumStr, buffer+inode_base, len);
                inodeNumStr[len] = '\0';
                int inodeNum = str_to_int(inodeNumStr, len);
                //printf("[get_inode_by_filename]file: %s 's inode num is %d\n", name, inodeNum);
                return inodeNum;
            }
            // 维护fileName_base的起始地址
            fileName_base = i+1;
        }
    }
}

// filename-inodeNum;filename-inodeNum……
static int next_inde_block = 0;
int naive_fs_access(const char *name)
{
    /* LAB 6 TODO BEGIN */
    /* BLANK BEGIN */
    char buffer[BLOCK_SIZE];
    sd_bread(SUPER_BLOCK, buffer);
    int buffer_size = strlen(buffer);
    //printf("[naive_fs_access]read buffer size: %d and buffer: %s\n", buffer_size, buffer);
    // 匹配文件名,目录格式:filename-inodeNum;filename-inodeNum;……
    int fileName_base = 0;
    int inode_base = 0;
    int find = 0;
    for (int i=0; i<buffer_size; ++i) {
        // 找到一个filename-inode映射，看看文件名是否是要找的那个
        if (buffer[i]=='-') {
            int j=fileName_base;
            for (; j<i; ++j) {
                if (buffer[j] != name[j-fileName_base]) {
                    break;
                }
            }
            // 如果是要找的文件，获取inodeNum后返回
            if (j == i) {
                find = 1;
            }
            // 维护inode_base的起始地址
            inode_base = i+1;
        }
        if (find == 1) {
            return 0;
        }
        // 找到一个filename-inode映射，看看是否需要获取inodeNum
        if (buffer[i]==';') {
            // 维护fileName_base的起始地址
            fileName_base = i+1;
        }
    }
    /* BLANK END */
    /* LAB 6 TODO END */
    return -1;
}

int naive_fs_creat(const char *name)
{
    /* LAB 6 TODO BEGIN */
    /* BLANK BEGIN */
    // 判断文件是否存在
    if (naive_fs_access(name) == 0) {
        printf("[naive_fs_creat]file %s already exist", name);
        return -1;
    }
    // 创建文件
    char buffer[BLOCK_SIZE];
    sd_bread(SUPER_BLOCK, buffer);
    int buffer_size = strlen(buffer);
    //printf("[naive_fs_creat]read buffer size: %d and buffer: %s\n", buffer_size, buffer);
    // 在目录末尾添加
    // 首先找到上一个文件的inode
    char inodeNumStr[128];
    int next_inodeNum = 1; 
    for (int i=buffer_size-1; i>0; --i) {
        if (buffer[i]=='-') {
            int len = buffer_size - i - 2;
            strncpy(inodeNumStr, buffer+i+1, len);
            next_inodeNum = str_to_int(inodeNumStr, len) + 1;
            break;
        }
    }
    //printf("[naive_fs_creat]next inode num:%s to %d\n",inodeNumStr, next_inodeNum);
    // 将创建的文件加入Buffer中
    char Next_InodeNum_str[128];
    int_to_str(next_inodeNum, Next_InodeNum_str);
    //printf("[naive_fs_creat]int_to_str inodeNum: %s\n", Next_InodeNum_str);
    int tmp = buffer_size;
    for (int i=0; i<strlen(name); ++i) {
        buffer[tmp] = name[i];
        tmp++;
    }
    buffer[tmp] = '-';
    tmp++;
    for (int i=0; i<strlen(Next_InodeNum_str); ++i) {
        buffer[tmp] =  Next_InodeNum_str[i];
        tmp++;
    }
    buffer[tmp] = ';';
    tmp++;
    
    //printf("[naive_fs_creat]buffer size change, before %d, after %d\n",buffer_size, strlen(buffer));
    // 写回
    sd_bwrite(SUPER_BLOCK, buffer);
    /* BLANK END */
    /* LAB 6 TODO END */
    return 0;
}

int naive_fs_pread(const char *name, int offset, int size, char *buffer)
{
    /* LAB 6 TODO BEGIN */
    /* BLANK BEGIN */
    // 判断offset是否合法
    if (offset > BLOCK_SIZE) {
        return 0;
    }
    //printf("[naive_fs_pread] get inode by filename\n");
    int inodeNum = get_inode_by_filename(name);
    char read_buffer[BLOCK_SIZE];
    sd_bread(inodeNum, read_buffer);
    // 不能用strncpy因为它碰见'\0'就截止了
    // strncpy(buffer, read_buffer+offset, size);
    for (int i=0; i<size; ++i) {
        buffer[i] = read_buffer[offset+i];
    }
    /* BLANK END */
    /* LAB 6 TODO END */
    return size;
}

int naive_fs_pwrite(const char *name, int offset, int size, const char *buffer)
{
    /* LAB 6 TODO BEGIN */
    /* BLANK BEGIN */
    // 判断offset是否合法
    if (offset > BLOCK_SIZE || size%BLOCK_SIZE != 0) {
        return 0;
    }
    //printf("[naive_fs_pwrite] get inode by filename\n");
    int inodeNum = get_inode_by_filename(name);
    sd_bwrite(inodeNum, buffer);
    /* BLANK END */
    /* LAB 6 TODO END */
    return size;
}

int naive_fs_unlink(const char *name)
{
    /* LAB 6 TODO BEGIN */
    /* BLANK BEGIN */
    char buffer[BLOCK_SIZE];
    sd_bread(SUPER_BLOCK, buffer);
    // 找到要删除的文件，删除后写回即可
    int buffer_size = strlen(buffer);
    //printf("[naive_fs_access]read buffer size: %d and buffer: %s\n", buffer_size, buffer);
    // 匹配文件名,目录格式:filename-inodeNum;filename-inodeNum;……
    int fileName_base = 0;
    int fileName_begin = 0;
    int inode_base = 0;
    int find = 0;
    for (int i=0; i<buffer_size; ++i) {
        // 找到一个filename-inode映射，看看文件名是否是要找的那个
        if (buffer[i]=='-') {
            int j=fileName_base;
            for (; j<i; ++j) {
                if (buffer[j] != name[j-fileName_base]) {
                    break;
                }
            }
            // 如果是要找的文件，获取inodeNum后返回
            if (j == i) {
                fileName_begin = fileName_base;
                find = 1;
            }
            // 维护inode_base的起始地址
            inode_base = i+1;
        }
        // 找到一个filename-inode映射，看看是否需要获取inodeNum
        if (buffer[i]==';') {
            if (find==1) {
                char new_buffer[BLOCK_SIZE];
                strncpy(new_buffer, buffer, fileName_begin);
                strcpy(new_buffer+fileName_begin, buffer+i+1);
                sd_bwrite(SUPER_BLOCK, new_buffer);
            }
            // 维护fileName_base的起始地址
            fileName_base = i+1;
        }
    }    
    /* BLANK END */
    /* LAB 6 TODO END */
    return 0;
}

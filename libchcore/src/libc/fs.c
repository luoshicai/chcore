/*
 * Copyright (c) 2022 Institute of Parallel And Distributed Systems (IPADS)
 * ChCore-Lab is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *     http://license.coscl.org.cn/MulanPSL
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v1 for more details.
 */

#include <stdio.h>
#include <string.h>
#include <chcore/types.h>
#include <chcore/fsm.h>
#include <chcore/tmpfs.h>
#include <chcore/ipc.h>
#include <chcore/internal/raw_syscall.h>
#include <chcore/internal/server_caps.h>
#include <chcore/procm.h>
#include <chcore/fs/defs.h>

typedef __builtin_va_list va_list;
#define va_start(v, l) __builtin_va_start(v, l)
#define va_end(v)      __builtin_va_end(v)
#define va_arg(v, l)   __builtin_va_arg(v, l)
#define va_copy(d, s)  __builtin_va_copy(d, s)


extern struct ipc_struct *fs_ipc_struct;

/* You could add new functions or include headers here.*/
/* LAB 5 TODO BEGIN */
int my_alloc_fd() {
    static int fd = 0;
    return fd++;
}
#define BUF_SIZE 1024

int my_open_file(const char *filename, FILE *f) {
    struct ipc_msg *ipc_msg =
        ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
    chcore_assert(ipc_msg);
    struct fs_request *fr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
    fr->req = FS_REQ_OPEN;
    strcpy(fr->open.pathname, filename);
    fr->open.flags = f->mode;
    fr->open.new_fd = f->fd;
    int ret = ipc_call(fs_ipc_struct, ipc_msg);
    ipc_destroy_msg(fs_ipc_struct, ipc_msg);
    return ret;
}

int my_create_file(const char *filename) {
    printf("create file %s\n", filename);
    struct ipc_msg *ipc_msg_create = ipc_create_msg(
        fs_ipc_struct, sizeof(struct fs_request), 0);
    chcore_assert(ipc_msg_create);
    struct fs_request *fr_create =
        (struct fs_request *)ipc_get_msg_data(ipc_msg_create);
    fr_create->req = FS_REQ_CREAT;
    strcpy(fr_create->creat.pathname, filename);
    int ret = ipc_call(fs_ipc_struct, ipc_msg_create);
    ipc_destroy_msg(fs_ipc_struct, ipc_msg_create);
    if (ret < 0)
        chcore_bug("create file failed!");
    return ret;
}

int my_lseek_file(FILE *f) {
    struct ipc_msg *ipc_msg = ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 1);
    struct fs_request *fr_ptr = (struct fs_request *) ipc_get_msg_data(ipc_msg);
    fr_ptr->req = FS_REQ_LSEEK;
    fr_ptr->lseek.fd = f->fd;
    fr_ptr->lseek.offset = f->offset;
    fr_ptr->lseek.whence = SEEK_SET;
    int ret = ipc_call(fs_ipc_struct, ipc_msg);
    ipc_destroy_msg(fs_ipc_struct, ipc_msg);
    if (ret < 0) {
        chcore_bug("lseek failed!");
    }
    return ret;
}

int get_num(char* buf, int* buf_p) {
    int num = 0;
    while (buf[*buf_p] >= '0' && buf[*buf_p] <= '9') {
        num = num * 10 + buf[*buf_p] - '0';
        (*buf_p)++;
    }
    return num;
}

void get_string(char *buf, int* buf_p, char* bind_data) {
    int string_start = *buf_p;
    while (buf[*buf_p] != ' ') {
        (*buf_p)++;
    }
    int len = *buf_p - string_start;
    chcore_assert(len > 0);
    memcpy(bind_data,(char *)buf + string_start,len);
    bind_data[len] = '\0';    
}

void int2str(int n, char *str){
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
/* LAB 5 TODO END */


FILE *fopen(const char *filename, const char *mode) {
    /* LAB 5 TODO BEGIN */
    FILE *fp = malloc(sizeof(FILE));
    int fd = my_alloc_fd();
    fp->fd = fd;
    fp->offset = 0;
    if (*mode == 'r') {
        fp->mode = O_RDONLY;        
    }
    else {
        fp->mode = O_WRONLY;
    }
    int ret = my_open_file(filename, fp);
    if (ret < 0) {
        if (*mode == 'r'){
            chcore_bug("open file failed:no such file");
        }
        else {
            my_create_file(filename);
            // reopen file
            ret = my_open_file(filename, fp);
        }
    }
    /* LAB 5 TODO END */
    return fp;
}

size_t fwrite(const void *src, size_t size, size_t nmemb, FILE *f)
{
    /* LAB 5 TODO BEGIN */
    if (f->mode == O_RDONLY) {
        chcore_bug("no access to write!");
    }
    // lseek
    my_lseek_file(f);
    // write
    size_t len = size * nmemb;
    int ret = 0;
    {
        struct ipc_msg *ipc_msg = ipc_create_msg(
            fs_ipc_struct, sizeof(struct fs_request) + len, 0);
        chcore_assert(ipc_msg);
        struct fs_request *fr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
        memcpy((void *)fr + sizeof(struct fs_request), src, len);
        fr->req = FS_REQ_WRITE;
        fr->write.count = len;
        fr->write.fd = f->fd;
        ret = ipc_call(fs_ipc_struct, ipc_msg);
        chcore_bug_on(ret != len);
        ipc_destroy_msg(fs_ipc_struct, ipc_msg);
    }
    f->offset += ret; 
    /* LAB 5 TODO END */
    return ret;
}

size_t fread(void *destv, size_t size, size_t nmemb, FILE *f)
{
        /* LAB 5 TODO BEGIN */
    // lseek
    my_lseek_file(f);
    // read
    size_t len = size * nmemb;
    int ret = 0;
    {
        struct ipc_msg *ipc_msg = ipc_create_msg(
            fs_ipc_struct, sizeof(struct fs_request) + len + 2, 0);
        chcore_assert(ipc_msg);
        struct fs_request *fr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
        fr->req = FS_REQ_READ;
        fr->read.fd = f->fd;
        fr->read.count = len;
        ret = ipc_call(fs_ipc_struct, ipc_msg);
        memcpy(destv, ipc_get_msg_data(ipc_msg), ret);
        ipc_destroy_msg(fs_ipc_struct, ipc_msg);
    }
    f->offset += ret; 
    /* LAB 5 TODO END */
    return ret;
}

int fclose(FILE *f)
{
    /* LAB 5 TODO BEGIN */
    struct ipc_msg *ipc_msg =
        ipc_create_msg(fs_ipc_struct, sizeof(struct fs_request), 0);
    chcore_assert(ipc_msg);
    struct fs_request *fr = (struct fs_request *)ipc_get_msg_data(ipc_msg);
    fr->req = FS_REQ_CLOSE;
    fr->close.fd = f->fd;
    int ret = ipc_call(fs_ipc_struct, ipc_msg);
    ipc_destroy_msg(fs_ipc_struct, ipc_msg);
    if (ret < 0) {
        chcore_bug("close file error");
    }
    free(f);
    /* LAB 5 TODO END */
    return 0;
}

/* Need to support %s and %d. */
int fscanf(FILE *f, const char *fmt, ...)
{
    /* LAB 5 TODO BEGIN */
    va_list va;
    va_start(va, fmt);
    char buf[BUF_SIZE] = {'\0'};
    int fmt_p = 0, buf_p = 0, fmt_size = strlen(fmt);
    int file_size = fread(buf, sizeof(char), BUF_SIZE, f);
    while (buf_p < file_size && fmt_p < fmt_size) {
        if (fmt[fmt_p] == '%') {
            fmt_p++;
            switch (fmt[fmt_p]) {
            case ('d'): {
                int *bind_data = va_arg(va, int *);
                *bind_data = get_num(buf, &buf_p);
                break;
            }
            case ('s'): {
                char *bind_data = va_arg(va, char *);
                get_string(buf, &buf_p, bind_data);
                break;
            }
            default: {
                chcore_bug("unsupported data type");
            }
            }
        fmt_p++;
        } else {
            chcore_assert(buf[buf_p] == fmt[fmt_p]);
            fmt_p++;
            buf_p++;
        }
    }
    va_end(va);
    /* LAB 5 TODO END */
    return 0;
}

/* Need to support %s and %d. */
int fprintf(FILE * f, const char * fmt, ...) {

    /* LAB 5 TODO BEGIN */
    char wbuf[512];
    memset(wbuf, 0x0, sizeof(wbuf));

    va_list va;
    va_start(va, fmt);
    printf("%s\n", fmt);
    int start = 0, i = 0;
    int offset = 0;
    while(i < strlen(fmt)){
        if(fmt[i] == '%'){
	    memcpy(wbuf + offset, fmt + start, i - start);
	    offset += i - start;
	    i++;
	    start = i + 1;
            switch (fmt[i]) {
                case 'd':{
	            int tmp = va_arg(va, int);
		    char str[256];
		    memset(str, '\0', sizeof(str));
		    int2str(tmp, str);
		    memcpy(wbuf + offset, str, strlen(str));
		    offset += strlen(str);                                
                    break;
                }
                case 's':{
		    char *tmp = va_arg(va, char *);
		    memcpy(wbuf + offset, tmp, strlen(tmp));
		    offset += strlen(tmp);   
                    break;                             
                }
                default:
                    chcore_bug("unsupported data type");
            }
	}
	i++;
    }
    printf("wbuf is %s ||\n", wbuf);
    fwrite(wbuf, sizeof(char), sizeof(wbuf), f);
    va_end(va);

    /* LAB 5 TODO END */
    return 0;
}



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

#include "shell.h"

#include <stdio.h>
#include <chcore/internal/raw_syscall.h>
#include <chcore/fs/defs.h>
#include <chcore/assert.h>

// get a character from standard input
char getch()
{
	int c;
	/* LAB 5 TODO BEGIN */
    c = chcore_console_getc();
	/* LAB 5 TODO END */
	return (char) c;
}

extern struct ipc_struct *fs_ipc_struct;

#ifdef SHELL_TEST
void shell_test();
#endif

#ifdef FSM_TEST
void fsm_test();
#endif

int main(int argc, char *argv[])
{
	char *buf;
	int ret = 0;

	printf("Run shell server !\n");

	connect_fs();

#ifdef SHELL_TEST
	shell_test();
#endif

#ifdef FSM_TEST
	fsm_test();
#endif

	printf("The shell_test finish!\n");
	
	while (1) {
		printf("\n");
		buf = readline("$ ");
		if (buf == NULL) {
			printf("buf is NULL\n");
			__chcore_sys_thread_exit();
		}
		if (buf[0] == 0)
			continue;
		if (builtin_cmd(buf))
			continue;
		if ((ret = run_cmd(buf)) < 0) {
			printf("Cannot run %s, ERROR %d\n", buf, ret);
		}
		// 在shell的每次输入后只有一小段时间是yield的，这导致cpu长时间被getch（）的内核态等待所占据
		// 老师建议在此加入死循环，等待测试脚本捕捉输出kill qemu进程, 只有这样才能过测试
		while (1) {
		
		}
		
	}

	return 0;
}



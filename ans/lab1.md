

## **OS-lab1：机器启动**

学号: 520021910605  姓名：罗世才



```
思考题 1：阅读 `_start` 函数的开头，尝试说明 ChCore 是如何让其中一个核首先进入初始化流程，并让其他核暂停执行的。
```

在_start函数开头的代码及每一行代码的作用如下所示：

	mrs	x8, mpidr_el1  //将core ID移到寄存器x8中
	and	x8, x8,	#0xFF  //mask，取x8的0~7位
	cbz	x8, primary    //将x8和0进行比较，如果结果为0就转移
	
	/* hang all secondary processors before we introduce smp */
	b 	.              //(branch)跳转到某地址(无返回), 不会改变lr(x30)寄存器的值。
	                   //b .则表示跳转到当前地址
[Arm Cortex-A75 Core Technical Reference Manual r3p1](https://developer.arm.com/documentation/100403/0301/register-descriptions/aarch64-system-registers/mpidr-el1--multiprocessor-affinity-register--el1?lang=en)中这样介绍mpidr_el1:

Affinity level 0. The level identifies individual threads within a multithreaded core.

所以mpdir_el1低8位对应的是Affinity level 0，且仅有一个核的这个字段为0，故只有这个核在cbz x8,primary后会跳转执行primary，进入初始化流程。而其他这个字段非0的核则会不断执行b . 语句而被挂起。



```
练习题 2：在 `arm64_elX_to_el1` 函数的 `LAB 1 TODO 1` 处填写一行汇编代码，获取 CPU 当前异常级别。
提示：通过 `CurrentEL` 系统寄存器可获得当前异常级别。通过 GDB 在指令级别单步调试可验证实现是否正确。
```

根据提示以及后文总是用x9寄存器进行比较可知，填写代码为：

```
mrs x9, CurrentEL
```

mrs将系统寄存器move到通用寄存器。

即通过"CurrentEL"系统寄存器获取当前异常级别并移至x9寄存器中，方便后续使用。

使用GDB进行验证：

![](./1-1.png)

从上图可以看出，在经过mrs x9, currentel后，x9寄存器的值为12。而反汇编代码中CURRENTEL_EL1被翻译为#0x4，CURRENTEL_EL2被翻译为#0x8，猜测x9的值为12应当表示我们处于EL3。



```
练习题 3：在 `arm64_elX_to_el1` 函数的 `LAB 1 TODO 2` 处填写大约 4 行汇编代码，设置从 EL3 跳转到 EL1 所需的 `elr_el3` 和 `spsr_el3` 寄存器值。具体地，我们需要在跳转到 EL1 时暂时屏蔽所有中断、并使用内核栈（`sp_el1` 寄存器指定的栈指针）。
```

填写代码为：

    adr x9, .Ltarget
    msr elr_el3, x9
    mov x9, SPSR_ELX_DAIF | SPSR_ELX_EL1H
    msr spsr_el3, x9
其中，前两行是为了设置elr_el3（异常链接寄存器），其控制异常返回后执行的指令地址，在第二行将其设置为.Ltarget后，eret时就会跳转到.Ltarget。

后两行是为了设置spsr_el3（保存的程序状态寄存器），其控制返回后应恢复的程序状态（包括异常返回后的异常级别），在这里SPSR_ELX_DAIF（D: debug; A: error; I: interrupt; F: fast interrupt）应该是表示跳转到 EL1 时暂时屏蔽所有中断，SPSR_ELX_EL1H应该是表示返回后的异常级别为el1。

综合起来产生的效果应该为：eret时跳转到.Ltarget同时进入EL1。

使用GDB进行验证：

![](./1-2.png)

从图中可以看出，0x80010 bl 0x88000<arm64_elX_to_el1>这一行应该是arm64_elX_to_el1的调用，在补上上面四行汇编后，程序执行流成功执行了函数调用行的下一行，说明可以成功从arm64_elX_to_el1返回_start。



```
思考题 4：结合此前 ICS 课的知识，并参考 `kernel.img` 的反汇编（通过 `aarch64-linux-gnu-objdump -S` 可获得），说明为什么要在进入 C 函数之前设置启动栈。如果不设置，会发生什么？
```

在终端中运行aarch64-linux-gnu-objdump -S build/kernel.img，结果如下图所示，进入init_c后的第一行代码为：

![](./1-3.png)

其中，x29是帧指针，x30是链接寄存器，在刚进入init_c时，这两个寄存器将会被压入栈中，以便结束后恢复并返回至原函数。如果没有设置启动栈，sp就是一个未知地址，可能带来错误。

此外，联系到ics所学，栈在函数调用中起着非常重要的作用，包括：

1. 传递参数；在arm中，x0~x7为参数寄存器，超出的参数则需通过栈来传递，故如果不设置栈，可能会导致参数的丢失等；

2. 保存局部变量；主要为函数的非静态局部变量，故如果不设置栈，可能会导致变量无法被正确访问或导致变量被篡改；

3. 保存函数调用前后上下文；如果不设置可能会导致上下文丢失。

   

  由此可见，在进入C函数前设置启动栈是非常必要的。



```
思考题 5：在实验 1 中，其实不调用 `clear_bss` 也不影响内核的执行，请思考不清理 `.bss` 段在之后的何种情况下会导致内核无法工作。
```

​    bss段主要存放的是未初始化或初始化为0的静态和全局变量，这些变量在编译的时候不会分配内存,可能在加载的时候会为之分配。在正常的情况下，bss段被加载到内存时会被操作系统自动初始化为0，但Lab所写的代码就是操作系统，所以我们需要自己clear_bss。

​     所以，我觉得如果在这里不清理bss，那么加载时分配的bss段里面就是一些随机的数据。如果在之后使用这些变量时没有先进行赋值而是依然假设其已为0，那肯定会出现一些问题导致内核无法工作。至于为什么实验1中不影响内核的执行，我认为可能是没有使用bss段中的变量或是代码很严谨，每个这种变量使用前都进行了检查或者赋值。



```
练习题 6：在 `kernel/arch/aarch64/boot/raspi3/peripherals/uart.c` 中 `LAB 1 TODO 3` 处实现通过 UART 输出字符串的逻辑。
```

填写代码为：

        early_uart_init();
        for (int i=0; str[i]!='\0'; ++i) {
                early_uart_send(str[i]);
        }
即先使用early_uart_init()进行初始化，再写一个循环通过early_uart_send()逐个输出字符。

填写代码后运行make qemu，得到如下结果：

![](./1-4.png)

这在写上面这段代码之前是没有的，说明这段代码成功输出了字符串。



```
练习题 7：在 `kernel/arch/aarch64/boot/raspi3/init/tools.S` 中 `LAB 1 TODO 4` 处填写一行汇编代码，以启用 MMU。
```

填写代码如下：

```
    orr     x8, x8, #SCTLR_EL1_M
```

老师上课的PPT中有这行代码，但老师上课时没有一行行代码的讲，我参考了[arm Developer](https://developer.arm.com/documentation/100403/0301/register-descriptions/aarch64-system-registers/sctlr-el1--system-control-register--el1?lang=en)中对SCTR_EL1的解释，结合Lab1 guide上面的提示，大概知道了：

![](./1-5.png)

我补上的第255行是设置M字段来启用MMU，第258~261行是启用对齐检查，第263~265行是启用指令和数据缓存，最后把设置好的结果移入sctlr_el1中从而启用MMU。

按照Lab1 Guide上的检测方法，发现执行流确实会在0x200地址处无限跳转：

![](./1-6.png)

![](./1-7.png)



最后make grade：

![](./1-8.png)



**参考**

[(74条消息) ARM 汇编指令_arm cbz_never_go_away的博客-CSDN博客](https://blog.csdn.net/never_go_away/article/details/126884707)

[再谈应用程序分段： 数据段、代码段、BSS段以及堆和栈 - 知乎 (zhihu.com)](https://zhuanlan.zhihu.com/p/348026261)


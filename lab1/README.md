# 文件说明
## 源文件
kern/init/entry.S: OpenSBI 启动之后将要跳转到的一段汇编代码,即整个程序最先执行的文件。在这里进行内核栈的分配，然后转入 C 语言编写的内核初始化函数。\
kern/init/init.c：C 语言编写的内核入口点。主要包含 kern_init() 函数，从 kern/entry.S 跳转过来，调用中断机制。\
kern/driver/console.c(h): 在 QEMU 上模拟的时候，唯一的“设备”是虚拟的控制台，通过 OpenSBI接口使用。简单封装了 OpenSBI 的字符读写接口，向上提供给输入输出库。\
kern/driver/clock.c(h): 通过 OpenSBI 的 接 口, 可 以 读 取 当 前 时 间 (rdtime), 设置时钟事件(sbi_set_timer)，是时钟中断必需的硬件支持。\
kern/driver/intr.c(h): 中断也需要 CPU 的硬件支持，这里提供了设置中断使能位的接口（其实只封装了一句 riscv 指令）。\
kern/trap/trapentry.S: 把中断入口点设置为这段汇编代码。这段汇编代码把寄存器的数据挪来挪去，进行上下文切换。\
kern/trap/trap.c(h): 分发不同类型的中断给不同的 handler, 完成上下文切换之后对中断的具体处理，例如外设中断要处理外设发来的信息，时钟中断要触发特定的事件。中断处理初始化的函数也在这里，主要是把中断向量表 (stvec) 设置成所有中断都要跳到 trapentry.S 进行处理。

## 库文件
libs/riscv.h: 以宏的方式，定义了 riscv 指令集的寄存器和指令。如果在 C 语言里使用 riscv 指令，需要通过内联汇编和寄存器的编号。这个头文件把寄存器编号和内联汇编都封装成宏，使得我们可以用类似函数的方式在 C 语言里执行一句 riscv 指令。\
libs/sbi.c(h): 封装 OpenSBI 接口为函数。如果想在 C 语言里使用 OpenSBI 提供的接口，需要使用内联汇编。这个头文件把 OpenSBI 的内联汇编调用封装为函数。\
libs/defs.h: 定义了一些常用的类型和宏。例如 bool 类型（C 语言不自带，这里 typedef int bool)。\
libs/string.c(h): 一些对字符数组进行操作的函数，如 memset(),memcpy() 等，类似 C 语言的string.h。\
kern/libs/stdio.c, libs/readline.c, libs/printfmt.c: 实现了一套标准输入输出，功能类似于C 语言的 printf() 和 getchar()。需要内核为输入输出函数提供两个桩函数（stub): 输出一个字符的函
数，输入一个字符的函数。在这里，是 cons_getc() 和 cons_putc()。\
kern/errors.h: 定义了一些内核错误类型的宏。

## 其他
tools/kernel.ld: ucore 的链接脚本 (link script), 告诉链接器如何将目标文件的 section 组合为可执行文件。\
tools/function.mk: 定义 Makefile 中使用的一些函数。\
Makefile: GNU make 编译脚本。


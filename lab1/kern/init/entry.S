# OpenSBI 启动之后将要跳转到的一段汇编代码。在这里进行内核栈的分配，然后转入C语言编写的内核初始化函数。


#include <mmu.h>
#include <memlayout.h>

    .section .text,"ax",%progbits   
    # 定义代码段，ax为代码段的属性，可执行可读。
    # %progbits是一个特殊的标志，用于指示该段包含的是程序的代码和数据，而不是未初始化的数据。
    .globl kern_entry  # 全局声明，使得ld能够看到kern_entry这个符号所在的位置

# 即在链接脚本中定义的入口点kern_entry
kern_entry:
    # 这条指令将bootstacktop的地址加载到栈指针sp中
    # 即bootstacktop作为标号，代表一个内存地址，通过.space又声明了KSTACKSIZE这么多字节的内存，即形成一个内核栈
    la sp, bootstacktop   

    tail kern_init      # 跳转到kern_init

.section .data
    # .align 2^12     
    .align PGSHIFT       # 按照2^PGSHIFT进行地址对齐,也就是对齐到下一页。PGSHIFT在mmu.h定义
    .global bootstack    # 内核栈
bootstack:
    .space KSTACKSIZE            # 留出KSTACKSIZE这么多个字节的内存
    .global bootstacktop         # 之后内核栈将要从高地址向低地址增长,初始时的内核栈为空
bootstacktop:
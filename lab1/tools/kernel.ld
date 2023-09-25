/* Simple linker script for the ucore kernel.
/* 链接本质：多个.o合成一个可执行文件，合并section，以及指定内存布局、分配等 */
/* 链接脚本 (linker script) 的作用，就是描述怎样把输入文件的 section 映射到输出文件的section, 同时规定这些 section 的内存布局。*/


OUTPUT_ARCH(riscv)            /* 指定输出文件的指令集架构,在riscv平台上运行 */
ENTRY(kern_entry)             /* 指定程序的入口点,是一个叫做kern_entry的符号 */
BASE_ADDRESS = 0x80200000;    /* 定义了一个变量BASE_ADDRESS并初始化 */

/* 链接脚本剩余的部分是一整条SECTIONS指令，用来指定输出文件的所有SECTION
"."是SECTIONS指令内的一个特殊变量/计数器，对应内存里的一个地址 */

/*开始进行节的分配和合并*/
SECTIONS
{
    /* Load the kernel at this address: "." means the current address */
    . = BASE_ADDRESS;

    /* 从.的当前值（当前地址）开始放置一个叫做text的section，用正则表达式进行匹配，
       如果输入文件中有一个section的名称符合花括号内部的格式那么这个section就被加
       到输出文件的text这个section里。输入文件中section的名称,有些是编译器自动生
       成的,有些是我们自己定义的 */

    .text : {           
        *(.text.kern_entry)         /* 把输入中kern_entry这一段放到输出中text的开头 */
        *(.text .stub .text.* .gnu.linkonce.t.*)
    }

    PROVIDE(etext = .); /* Define the 'etext' symbol to this value */

    .rodata : {
        *(.rodata .rodata.* .gnu.linkonce.r.*)
    }

    /* 进行地址对齐， 将 "."增加到0x1000的整数倍，也就是下一个内存页的起始处 */
    . = ALIGN(0x1000);

    /* The data segment */
    .data : {
        *(.data)
        *(.data.*)
    }

    /* small data section,存储字节数小于某个标准的变量，一般是char,short等类型 */
    .sdata : {
        *(.sdata)
        *(.sdata.*)
    }

    PROVIDE(edata = .);

    .bss : {
        *(.bss)
        *(.bss.*)
        *(.sbss*)
    }

    PROVIDE(end = .);

    /* /DISCARD/表示忽略，输入文件里*(.eh_frame .note.GNU-stack)这些section都被忽略，不会加入到输出文件中 */
    /* 部分节产生于编译过程中，在链接时不需要再输出到可执行文件中 */
    /DISCARD/ : {
        *(.eh_frame .note.GNU-stack)
    }
}

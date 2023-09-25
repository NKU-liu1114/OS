# 保存和恢复上下文，并把上下文包装成结构体送到trap 函数里去
#include <riscv.h>

    .macro SAVE_ALL                   # 定义汇编宏，类似于高级语言的内联函数

    csrw sscratch, sp                 # 保存原先的栈顶指针到sscratch，以待对sp特殊处理，否则无法上下文切换时无法恢复sp

    addi sp, sp, -36 * REGBYTES       # REGBYTES是riscv.h定义的常量，表示一个寄存器占据几个字节。
                                      # 将栈顶指针向上移动36个寄存器大小的位置，用于存储36个寄存器，保存上下文（一个trapFrame结构体）
                                      # 32个同游寄存器+4个和中断相关的CSR（Control and Status Registers）

    # 保存除sp外的通用寄存器
    STORE x0, 0*REGBYTES(sp)
    STORE x1, 1*REGBYTES(sp)
    STORE x3, 3*REGBYTES(sp)
    STORE x4, 4*REGBYTES(sp)
    STORE x5, 5*REGBYTES(sp)
    STORE x6, 6*REGBYTES(sp)
    STORE x7, 7*REGBYTES(sp)
    STORE x8, 8*REGBYTES(sp)
    STORE x9, 9*REGBYTES(sp)
    STORE x10, 10*REGBYTES(sp)
    STORE x11, 11*REGBYTES(sp)
    STORE x12, 12*REGBYTES(sp)
    STORE x13, 13*REGBYTES(sp)
    STORE x14, 14*REGBYTES(sp)
    STORE x15, 15*REGBYTES(sp)
    STORE x16, 16*REGBYTES(sp)
    STORE x17, 17*REGBYTES(sp)
    STORE x18, 18*REGBYTES(sp)
    STORE x19, 19*REGBYTES(sp)
    STORE x20, 20*REGBYTES(sp)
    STORE x21, 21*REGBYTES(sp)
    STORE x22, 22*REGBYTES(sp)
    STORE x23, 23*REGBYTES(sp)
    STORE x24, 24*REGBYTES(sp)
    STORE x25, 25*REGBYTES(sp)
    STORE x26, 26*REGBYTES(sp)
    STORE x27, 27*REGBYTES(sp)
    STORE x28, 28*REGBYTES(sp)
    STORE x29, 29*REGBYTES(sp)
    STORE x30, 30*REGBYTES(sp)
    STORE x31, 31*REGBYTES(sp)

    # get sr, epc, badvaddr, cause
    # Set sscratch register to 0, so that if a recursive exception
    # occurs, the exception vector knows it came from the kernel
    # RISCV不能直接从CSR写到内存, 需要csrr把CSR读取到通用寄存器，再从通用寄存器STORE回内存

    # CSR存到通用寄存器中
    csrrw s0, sscratch, x0           # s0=sscratch   sscratch=x0=0 即存储sp寄存器
    csrr s1, sstatus                 # s1=sstatus
    csrr s2, sepc                    # s2=sepc
    csrr s3, sbadaddr                # s3=sbadaddr
    csrr s4, scause
    
    # 通用寄存器再存回内存
    STORE s0, 2*REGBYTES(sp)    
    STORE s1, 32*REGBYTES(sp)
    STORE s2, 33*REGBYTES(sp)
    STORE s3, 34*REGBYTES(sp)
    STORE s4, 35*REGBYTES(sp)
    .endm                            # 汇编宏结束

    .macro RESTORE_ALL               # 定义恢复段的汇编宏

    # 之前保存的几个CSR并不都需要恢复,只恢复sepc和sstatus
    # 内存到通用寄存器
    LOAD s1, 32*REGBYTES(sp)
    LOAD s2, 33*REGBYTES(sp)
    
    # 通用寄存器到CSR
    csrw sstatus, s1
    csrw sepc, s2

    # 恢复除sp外的通用寄存器
    LOAD x1, 1*REGBYTES(sp)
    LOAD x3, 3*REGBYTES(sp)
    LOAD x4, 4*REGBYTES(sp)
    LOAD x5, 5*REGBYTES(sp)
    LOAD x6, 6*REGBYTES(sp)
    LOAD x7, 7*REGBYTES(sp)
    LOAD x8, 8*REGBYTES(sp)
    LOAD x9, 9*REGBYTES(sp)
    LOAD x10, 10*REGBYTES(sp)
    LOAD x11, 11*REGBYTES(sp)
    LOAD x12, 12*REGBYTES(sp)
    LOAD x13, 13*REGBYTES(sp)
    LOAD x14, 14*REGBYTES(sp)
    LOAD x15, 15*REGBYTES(sp)
    LOAD x16, 16*REGBYTES(sp)
    LOAD x17, 17*REGBYTES(sp)
    LOAD x18, 18*REGBYTES(sp)
    LOAD x19, 19*REGBYTES(sp)
    LOAD x20, 20*REGBYTES(sp)
    LOAD x21, 21*REGBYTES(sp)
    LOAD x22, 22*REGBYTES(sp)
    LOAD x23, 23*REGBYTES(sp)
    LOAD x24, 24*REGBYTES(sp)
    LOAD x25, 25*REGBYTES(sp)
    LOAD x26, 26*REGBYTES(sp)
    LOAD x27, 27*REGBYTES(sp)
    LOAD x28, 28*REGBYTES(sp)
    LOAD x29, 29*REGBYTES(sp)
    LOAD x30, 30*REGBYTES(sp)
    LOAD x31, 31*REGBYTES(sp)

    # 最后恢复sp
    LOAD x2, 2*REGBYTES(sp)
    #addi sp, sp, 36 * REGBYTES
    .endm   # 恢复的汇编宏结束

    .globl __alltraps   # 全局符号
.align(2)               # 内存对齐2^2即4字节
__alltraps:
    SAVE_ALL            # 保存上下文

    move  a0, sp        # 传递参数，a0寄存器传递参数给接下来调用的函数trap
    # trap是trap.c里面的一个C语言函数，也就是中断处理程序
    jal trap
    # trap函数执行完之后，会回到这里向下继续执行__trapret里面的内容

    .globl __trapret
__trapret:              # 异常处理结束后的返回点
    RESTORE_ALL         # 恢复上下文
    sret                # 根据sepc返回异常点

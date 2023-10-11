# 操作系统Lab1实验报告

## 一、实验要求
• 基于 markdown 格式来完成，以文本方式为主\
• 填写各个基本练习中要求完成的报告内容\
• 完成实验后，请分析 ucore_lab 中提供的参考答案，并请在实验报告中说明你的实现与参考答案的区别\
• 列出你认为本实验中重要的知识点，以及与对应的 OS 原理中的知识点，并简要说明你对二者的含义，
关系，差异等方面的理解（也可能出现实验中的知识点没有对应的原理知识点）\
• 列出你认为 OS 原理中很重要，但在实验中没有对应上的知识点

## 二、实验内容
### 练习 1：理解内核启动中的程序入口操作
阅读 kern/init/entry.S 内容代码，结合操作系统内核启动流程，说明指令 la sp, bootstacktop 完成了什么操作，
目的是什么？tail kern_init 完成了什么操作，目的是什么？

la sp,bootstacktop将bootstacktop的地址赋值给sp寄存器，目的是构造内核栈。由于在声明KSTACKSIZE这么多字节的空间后紧接着声明了bootstacktop，这些
空间刚好构成内核栈。tail kern_init 指令跳转到了init.c的kern_init函数中，即真正的入口点。

### 练习 2：完善中断处理（需要编程）
请编程完善 trap.c 中的中断处理函数 trap，在对时钟中断进行处理的部分填写 kern/trap/trap.c 函数中处理时钟中断的部分，使操作系统每遇到 100 次时钟中断后，调用 print_ticks 子程序，向屏幕上打印一行文字”100ticks”，在打印完 10 行后调用 sbi.h 中的 shut_down() 函数关机。\
要求完成问题 1 提出的相关函数实现，提交改进后的源代码包（可以编译执行），并在实验报告中简要说明实现过程和定时器中断处理的流程。实现要求的部分代码后，运行整个系统，大约每 1 秒会输出一次”
100 ticks”，输出 10 行。


时钟中断部分代码如下：
```
case IRQ_S_TIMER:
            clock_set_next_event();
            if (++ticks % TICK_NUM == 0) {
                print_ticks();   // 每发生100次时钟中断输出一行100tick
                n++;
            }
            if(n==10){
                sbi_shutdown();
            }
            break;
```
实现过程即是通过clock_set_next_event()函数通过sbi来产生时钟中断，当产生时钟中断时，程序会进入trapentry.S中执行，通过一个结构体来保存所有寄存器和CSR的值。完成上下文的保存后会进入trap.c中的trap函数，trap函数会调用trap_dispatch()，同时通过一个cause来分发是执行中断处理函数还是异常处理函数。时钟中断属于中断，进入中断处理程序，之后再根据cause执行对应的中断处理。在中断处理中首先设置下一次中断的时间，当完成100次中断处理后输出一次100ticks。最后回到trapentry.S中继续执行恢复上下文的操作。

### Challenge1：描述与理解中断流程
回答：描述 ucore 中处理中断异常的流程（从异常的产生开始），其中 mov a0，sp 的目的是什么？SAVE_ALL
中寄存器保存在栈中的位置是什么确定的？对于任何中断，__alltraps 中都需要保存所有寄存器吗？请说明
理由。

产生中断异常后，ucore会进入到trapentry.S中保存上下文，保存的过程需要用到内核栈。同时由于CSR不能直接保存，需要先存到通用寄存器中再加载到内存里（恢复过程与此相反）。然后调用trap()函数，根据中断/异常以及中断异常的类型进入相应的处理函数（通过cause来标识）。处理完成后回到trapentry.S中，完成上下文的恢复并返回中断异常点。\
mov a0,sp 的作用是将sp的值赋给a0,，ao寄存器用于函数调用时的参数传递。而sp是内核栈的栈顶地址，位于栈中的是trapframe结构体，即所有CSR和通用寄存器的值。结构体在内存中即是一片连续的数据空间，a0指向栈顶，在调用trap函数时即可将栈中的结构体赋给对应的参数tf，完成结构体的参数传递。\
SAVE_ALL中寄存器保存在栈中的位置是由trapframe结构体的定义确定的，因为接下来会将栈顶的地址作为结构体指针传入trap函数，所以栈中位置应与结构体的定义一一匹配。trapentry.S的入栈过程和结构体的声明也验证了这一点。\
__alltraps需要保存所有寄存器。对于某一具体的中断处理程序，实际上不需要保存没有使用和修改的寄存器。但作为通用的上下文切换程序，__alltrape又需要保存所有寄存器的值。这是因为对于每一个不同的异常和中断，不可能编写与之对应的上下文切换程序。这样做效率太低，只需一个通用的上下文切换程序即可，



### Challenge2：理解上下文切换机制
回答：在 trapentry.S 中汇编代码 csrw sscratch, sp；csrrw s0, sscratch, x0 实现了什么操作，目的是什么？save all
里面保存了 stval scause 这些 csr，而在 restore all 里面却不还原它们？那这样 store 的意义何在呢？

csrw sscratch,sp 指令将sp的值写入sscratch，之后开辟栈帧用于保存trapframe结构体，防止因为调整栈帧丢失原有的sp的值。sscratch初始值为0（即中断前处于S态）。\
csrrw s0, sscratch, x0 指令将sscratch的值赋给s0通用寄存器，然后再将s0的值保存到内存中（内核栈）上。再将x0（零寄存器）赋值给sscratch，即恢复sscratch的初始值，保持S态。\
stval和scause等寄存器可作为结构体传入中断/异常处理函数。在处理异常时，操作系统会读取 stval 寄存器的值来获取导致异常的额外信息。有助于操作系统更好地理解异常的原因并作出相应的处理；在进入trap_dispatch函数后，也需要根据scause的值来确定是调用中断处理函数还是异常处理函数，以及进一步根据scause来确定是什么类型的中断或异常，进而执行相应的操作。所以需要save all中保存下来，但恢复时不需要恢复它们，不恢复并不影响程序返回异常中断点继续执行。


### Challenge3：完善异常中断
编程完善在触发一条非法指令异常 mret 和，在 kern/trap/trap.c 的异常处理函数中捕获，并对其进行处理，简
单输出异常类型和异常指令触发地址，即“Illegal instruction caught at 0x(地址)”，“ebreak caught at 0x（地址）”
与“Exception type:Illegal instruction”，“Exception type: breakpoint”。

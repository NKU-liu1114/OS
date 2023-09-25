# 操作系统Lab1实验报告

### 练习1

阅读 kern/init/entry.S 内容代码，**结合操作系统内核启动流程**，说明**指令 la sp, bootstacktop 完成了什么操作**，目的是什么？**tail kern_init 完成了什么操作**，目的是什么？



#### 内核启动流程

进入lab1文件夹下，执行
```shell
make debug
make gdb

```
然后执行
```shell
x/10i $pc
```

可以看到当前pc所在位置0x00001000所对应的10条指令

它们依次是：

```shell
=> 0x1000:	auipc	t0,0x0
   0x1004:	addi	a2,t0,40
   0x1008:	csrr	a0,mhartid
   0x100c:	ld	a1,32(t0)
   0x1010:	ld	t0,24(t0)
   0x1014:	jr	t0
   0x1018:	unimp
   0x101a:	0x8000
   0x101c:	unimp
   0x101e:	unimp
```

**上述所有操作实现的效果是：让pc跳转到0x80000000**



其中**OpenSBI.bin**被加载到物理内存以物理地址 **0x80000000** 开头的区域上，同时**内核镜像** **os.bin** 被加载到以物理地址 **0x80200000** 开头的区域上。

其中**0x80000000的指令执行了OpenSBI的程序**，我们可以在0x80200000打一个断点，从0x80000000开始执行到断点处，可以看到执行到**0x80200000的第一条指令(内核的第一条指令)**：

**la sp,bootstacktop**

（lab0和lab1这些代码都是一样的）

而这一条指令恰好对应着**kern/init/entry.s**(内核/初始化/内核入口)当中kern_entry的指令


对于代码的解析：

1. `kern_entry:`内核入口点。
2. `la sp, bootstacktop`: 使用 **`la`**（load address）指令，**将 `sp`**（栈指针寄存器）**设置为 `bootstacktop` 的地址**，将**栈指针设置为栈顶部**。这一步操作**初始化了内核的栈**，以**准备**处理函数调用和中断。
3. `tail kern_init`: 使用 **`tail`** 指令，**跳转到 `kern_init` 标签所指位置**。这是内核初始化的入口，一旦内核的栈准备好，控制权就传递给了 `kern_init`，**开始执行内核的初始化**过程。




### 练习1的答案

因此，我们得出结论,在kern/init/entry.s当中：
（1）**la sp, bootstacktop 完成的操作是：**

**让sp=0x80204000**



**目的是：**
将**栈指针设置为栈顶部**，**初始化了内核的栈**，以**准备**处理函数调用和中断。由于在声明**KSTACKSIZE**这么多字节的空间后紧接着声明了bootstacktop，这些空间刚好构成内核栈。


（2）**tail kern_init 完成的操作是**

**让pc跳转到0x8020000c,去执行init.c里面的代码memset(edata, 0, end - edata);**



**目的是：**

控制权就传递给`kern_init`，即真正的入口点，**开始执行内核的初始化**过程

### 练习2

编程完善 trap.c 中的中断处理函数 trap，在对时钟中断进行处理的部分填写 kern/trap/trap.c 函数中处理时

钟中断的部分，使操作系统每遇到 100 次时钟中断后，调用 print_ticks 子程序，向屏幕上打印一行文字”100

ticks”，在打印完 10 行后调用 sbi.h 中的 shut_down() 函数关机。要求完成问题 1 提出的相关函数实现，

提交改进后的源代码包（可以编译执行），并在实验报告中简要说明实现过程和定时器中断处理的流程。实现要求

的部分代码后，运行整个系统，大约每 1 秒会输出一次”100 ticks”，输出 10 行。



首先需要定义一个

```c
volatile size_t print_time=0;
```

记录打印的次数

然后根据注释写出代码即可：
```C++
            /*(1)设置下次时钟中断- clock_set_next_event()
             *(2)计数器（ticks）加一
             *(3)当计数器加到100的时候，我们会输出一个`100ticks`表示我们触发了100次时钟中断，同时打印次数（num）加一
            * (4)判断打印次数，当打印次数为10时，调用<sbi.h>中的关机函数关机
            */
            //
	//下面是我写的代码，根据要求进行了翻译
    		clock_set_next_event();
    		num++;
    		if (num == TICK_NUM) {
        		print_ticks();
        		num = 0; // 重置计数器
        		print_time++;//这里注意是先++后判断关机
        		if (print_time == 10) {
            		sbi_shutdown(); // 关机
        		}
    		}
    		break;
```




在lab1文件夹下面：
```shell
make qemu
```

大约每隔一秒输出一次100ticks，总共输出十次。实现过程即是通过clock_set_next_event()函数通过sbi来产生

时钟中断，当产生时钟中断时，程序会进入trapentry.S中执行，通过一个结构体来保存所有寄存器和CSR的值。完

成上下文的保存后会进入trap.c中的trap函数，trap函数会调用trap_dispatch()，同时通过一个cause来分发是

执行中断处理函数还是异常处理函数。时钟中断属于中断，进入中断处理程序，之后再根据cause执行对应的中断处

理。在中断处理中首先设置下一次中断的时间，当完成100次中断处理后输出一次100ticks。最后回到trapentry.S

中继续执行恢复上下文的操作。


### Challenge1：描述与理解中断流程

回答：描述 ucore 中处理**中断异常的流程**（从**异常的产生**开始），其中 mov a0，sp 的目的是什么？SAVE_ALL

中寄寄存器保存在栈中的位置是什么确定的？对于任何中断，__alltraps 中都需要保存所有寄存器吗？请说明

理由。


**异常的产生**

**开机后，会依次执行init/enrty.s,init/init.c**

**而在init.c里面，会执行clock_init()**

首先执行第一句话：**set_csr(sie, MIP_STIP)**;其中csr是control status register（状态控制寄存器），

STIP - Supervisor Timer Interrupt Pending是计时中断标志位，这句话设置了状态寄存器的计时中断标志位

为1，意味着**当计时器产生中断时，内核将能够捕获到这个中断**。

然后执行了**clock_set_next_event()**

这个函数的定义如下：

```c
void clock_set_next_event(void) {
     sbi_set_timer(get_cycles() + timebase);
 }
```

调用了kern/libs/sbi.c下面的函数sbi_set_timer，它的效果是调用sbi_call而这里的
**ecall(environment)会触发中断**。并且触发中断的时间是由get_cycles() + timebase传入的，相当于
**timebase这么多个周期后**就会触发中断，**如果触发中断的时候再次设置clock_set_next_event()，那么每隔timebase这么多个周期后就会反复触发中断。**而timebase=100000=10^5，而QEMU的时钟周期是10MHz也就是10^7，也就是0.01s之后就会触发中断，每秒触发100次中断。这一点可以被实验验证，**如果把timebase改为10^4,触发tick的速度将会变成原来的10倍**。

总而言之，**clock_init()的效果是：在0.01s之后触发一次中断，后面中断会不断迭代下去。**


出现中断（异常）后，CPU会跳到**stvec**寄存器，而stvec寄存器指向**alltraps**，alltraps在trapentry.S内被定义

CPU会进入到trapentry.S中保存上下文，保存的过程需要用到内核栈。同时由于CSR不能直接保存，需要先存到通用

存器中再加载到内存里（恢复过程与此相反）。然后调用**trap()**函数，根据中断/异常以及中断异常的类型进入相应

的处理函数（通过cause来标识）。处理完成后回到trapentry.S中，完成上下文的恢复并返回中断异常点。



**问题**：mov a0，sp 的目的是什么？

**为了传递 `sp` 的值给 `trap` 函数作为参数。**

sp恰好指向trapframe的起始地址，sp+0对应着zero寄存器，sp+1*registerSize对应着ra寄存器。

**mov a0,sp**的作用是将sp的值赋给a0,，ao寄存器用于函数调用时的参数传递。而sp是内核栈的栈顶地址，

位于栈中的是trapframe结构体，即所有CSR和通用寄存器的值。结构体在内存中即是一片连续的数据空间，

a0指向栈顶，在调用trap函数时即可将栈中的结构体赋给对应的参数tf，完成结构体的参数传递。


**问题**：SAVE_ALL中寄存器保存在栈中的位置是什么确定的？

**是在trapentry.s的 `SAVE_ALL` 宏结合trap.h的trapframe确定的**

SAVE_ALL中寄存器保存在栈中的位置是由trapframe结构体的定义确定的，因为接下来会将栈顶的地址作为结构体指针传

入trap函数，所以栈中位置应与结构体的定义一一匹配。trapentry.S的入栈过程和结构体的声明也验证了这一点。


**问题**：对于任何中断，__alltraps 中都需要保存所有寄存器吗？

__alltraps需要保存所有寄存器。对于某一具体的中断处理程序，实际上不需要保存没有使用和修改的寄存器。但作为

通用的上下文切换程序，__alltraps又需要保存所有寄存器的值。这是因为对于每一个不同的异常和中断，不可能编写

与之对应的上下文切换程序。这样做效率太低，只需一个通用的上下文切换程序即可。





### Challenge2

在 trapentry.S 中汇编代码 csrw sscratch, sp；csrrw s0, sscratch, x0 实现了什么操作，目的是什么？

`csrw sscratch, sp` 的操作是将 **`sp` 寄存器的值写入 `sscratch` 寄存器**。之后开辟栈帧用于保存

trapframe结构体，防止因为调整栈帧丢失原有的sp的值。sscratch初始值为0（即中断前处于S态）

`csrrw s0, sscratch, x0` 则是将 `s0` 寄存器设置为 `sscratch` 寄存器的值，**并且让sscratch=0，表示进入到了内核态**。

而sscratch的值就是sp的值，这个操作**将 `sp` 寄存器的值保存在栈上**。【这里很巧妙，STORE s0, 2*REGBYTES(sp)说明把sp寄存器保存到了栈上对应sp的值】



**问题**：save all里面保存了 stval scause 这些 csr，而在 restore all 里面却不还原它们？那这样 store 的意义何在呢？

 **stval**和**scause**等寄存器可作为结构体传入中断/异常处理函数。在处理异常时，操作系统会读取 stval 寄存器的值来获

 取导致异常的额外信息。有助于操作系统更好地理解异常的原因并作出相应的处理；在进入trap_dispatch函数后，也需

 要根据scause的值来确定是调用中断处理函数还是异常处理函数，以及进一步根据scause来确定是什么类型的中断或异常，

 进而执行相应的操作。所以需要save all中保存下来，但恢复时不需要恢复它们，不恢复并不影响程序返回异常中断点继续执行。



### Challenge 3

编程完善在触发一条非法指令异常 mret 和，在 kern/trap/trap.c 的异常处理函数中捕获，并对其进行处理，简

单输出异常类型和异常指令触发地址，即“Illegal instruction caught at 0x(地址)”，“ebreak caught at 0x（地址）”

与“Exception type:Illegal instruction”，“Exception type: breakpoint”。

**在trap.c内的exceptionHandler，写上中断处理程序:**

输出一句话**Illegal instruction exception written by dyx**

输出异常指令地址

跳过异常指令

        case CAUSE_ILLEGAL_INSTRUCTION:
             // 非法指令异常处理
             /* LAB1 CHALLENGE3   YOUR CODE :  */
            /*(1)输出指令异常类型（ Illegal instruction）
             *(2)输出异常指令地址
             *(3)更新 tf->epc寄存器
            */
            //下面是我写的代码，根据要求进行了翻译
             cprintf("Illegal instruction exception written by dyx\n");
    		cprintf("Exception program counter (epc): %p\n", tf->epc);
    		tf->epc += 4; // 更新 epc 寄存器，跳过异常指令
    	    //=================================================
            break;
        case CAUSE_BREAKPOINT:
            //断点异常处理
            /* LAB1 CHALLLENGE3   YOUR CODE :  */
            /*(1)输出指令异常类型（ breakpoint）
             *(2)输出异常指令地址
             *(3)更新 tf->epc寄存器
            */
            //下面是我写的代码，根据要求进行了翻译
                cprintf("Breakpoint exception\n");
    	cprintf("Exception program counter (epc): %p\n", tf->epc);
    		tf->epc += 4; // 更新 epc 寄存器，跳过异常指令
    	    //========================================

然后在init.c内触发异常:
```c
        __asm__ __volatile__(
	"mret");
```

之后make qemu 就可以输出异常处理



### 知识点总结

**练习1：内核启动流程**
1. 操作系统内核启动流程，从复位地址、到opensbi启动代码、到内核初始化流程。
2. 理解启动代码的初始化操作，如将栈指针设置为栈顶、跳转到内核初始化入口。

**练习2：处理时钟中断**
3. 时钟中断的触发和处理，包括设置中断标志位、计时器设置、中断触发时的处理。
4. 实现每100次时钟中断触发时打印"100 ticks"并在10次打印后关闭操作系统。

**Challenge 1：理解中断流程**
5. 中断的处理流程，包括中断触发、中断处理函数的调用。
6. 如何保存和还原寄存器，以及何时需要保存所有寄存器。

**Challenge 2：异常处理**
8. 区分中断和异常，了解异常的处理当中栈的操作。

**Challenge 3：异常处理**
11. 更深入了解异常处理流程，包括异常触发、异常处理函数调用、异常处理程序的编写。
12. 输出异常信息，如异常类型和异常指令触发地址。


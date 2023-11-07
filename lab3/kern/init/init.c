#include <defs.h>
#include <stdio.h>
#include <string.h>
#include <console.h>
#include <kdebug.h>
#include <trap.h>
#include <clock.h>
#include <intr.h>
#include <pmm.h>
#include <vmm.h>
#include <ide.h>
#include <swap.h>
#include <kmonitor.h>

int kern_init(void) __attribute__((noreturn));
void grade_backtrace(void);


int
kern_init(void) {
    extern char edata[], end[];
    memset(edata, 0, end - edata);

    const char *message = "(THU.CST) os is loading ...";
    cprintf("%s\n\n", message);

    print_kerninfo();

    // grade_backtrace();

    pmm_init();                 // init physical memory management
                                // 我们加入了多级页表的接口和测试

    idt_init();                 // init interrupt descriptor table

    vmm_init();                 // 进行虚拟内存管理机制的初始化。在此阶段，主要是建立虚拟地址到物理地址的映射关系，为虚拟内存提供管理支持

    ide_init();                 // 完成对用于页面换入和换出的硬盘（通常称为swap硬盘）的初始化工作
                                // 其实这个函数啥也没做, 属于"历史遗留"
    swap_init();                // 用于初始化页面置换算法，这其中包括Clock页替换算法的相关数据结构和初始化步骤

    clock_init();               // init clock interrupt
   // intr_enable();              // enable irq interrupt



    /* do nothing */
    while (1);
}

void __attribute__((noinline))
grade_backtrace2(int arg0, int arg1, int arg2, int arg3) {
    mon_backtrace(0, NULL, NULL);
}

void __attribute__((noinline))
grade_backtrace1(int arg0, int arg1) {
    grade_backtrace2(arg0, (sint_t)&arg0, arg1, (sint_t)&arg1);
}

void __attribute__((noinline))
grade_backtrace0(int arg0, sint_t arg1, int arg2) {
    grade_backtrace1(arg0, arg2);
}

void
grade_backtrace(void) {
    grade_backtrace0(0, (sint_t)kern_init, 0xffff0000);
}

static void
lab1_print_cur_status(void) {
    static int round = 0;
    round ++;
}



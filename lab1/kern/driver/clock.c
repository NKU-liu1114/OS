#include <clock.h>
#include <defs.h>
#include <sbi.h>
#include <stdio.h>
#include <riscv.h>
//time寄存器也是CSR，用于存储从系统启动开始经过的时钟周期数。
volatile size_t ticks;

//对64位和32位架构，读取time的方法是不同的
//32位架构下，需要把64位的time寄存器读到两个32位整数里，然后拼起来形成一个64位整数
//64位架构简单的一句rdtime就可以了
//__riscv_xlen是gcc定义的一个宏，可以用来区分是32位还是64位 。
static inline uint64_t get_cycles(void) {
#if __riscv_xlen == 64  //如果是RISCV64的架构
    uint64_t n;
    __asm__ __volatile__("rdtime %0" : "=r"(n)); //用于读取当前时间计数器，即把time寄存器里的值赋给n
    return n;
#else //RISCV32的架构
    uint32_t lo, hi, tmp;
    __asm__ __volatile__(          
        "1:\n"
        "rdtimeh %0\n"
        "rdtime %1\n"
        "rdtimeh %2\n"
        "bne %0, %2, 1b"
        : "=&r"(hi), "=&r"(lo), "=&r"(tmp));
    return ((uint64_t)hi << 32) | lo;
#endif
}


// Hardcode timebase，即每次时钟中断的间隔
static uint64_t timebase = 100000;

/* *
 * clock_init - initialize 8253 clock to interrupt 100 times per second,
 * and then enable IRQ_TIMER.
 * */
void clock_init(void) {
    // sie这个CSR可以单独使能禁用某个来源的中断。默认时钟中断是关闭的
    // 在初始化的时候，使能时钟中断
    set_csr(sie, MIP_STIP);
    // divided by 500 when using Spike(2MHz)
    // divided by 100 when using QEMU(10MHz)
    // timebase = sbi_timebase() / 500;
    //设置第一个时钟中断事件
    clock_set_next_event();

    // 初始化一个计数器
    ticks = 0;

    cprintf("++ setup timer interrupts\n");
}
//设置时钟中断：timer的数值变为当前时间 + timebase后，触发一次时钟中断
//对于QEMU，timer增加1，过去了10^-7s，也就是100ns
void clock_set_next_event(void) { sbi_set_timer(get_cycles() + timebase); }

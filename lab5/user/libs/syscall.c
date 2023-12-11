#include <defs.h>
#include <unistd.h>
#include <stdarg.h>
#include <syscall.h>

//最大可变参数
#define MAX_ARGS            5

// 类似于一个通用接口，sys_putc、sys_wait之类的函数都在此基础上进行封装\
// 在用户态进行系统调用的核心操作是，通过内联汇编进行ecall环境调用。这将产生一个trap, 进入S mode进行异常处理。
static inline int
syscall(int64_t num, ...) {
    // num对应定义的一系列系统调用的宏
    //va_list, va_start, va_arg都是C语言处理参数个数不定的函数的宏
    //在stdarg.h里定义
    va_list ap;//ap: 参数列表(此时未初始化)
    va_start(ap, num);//初始化参数列表, 从num开始
    uint64_t a[MAX_ARGS];//定义一个数组 a，用于存储系统调用的参数。
    int i, ret;
    for (i = 0; i < MAX_ARGS; i ++) {//把参数依次取出
        a[i] = va_arg(ap, uint64_t);
    }
    va_end(ap);

    asm volatile (
        //将系统调用号 num 和参数 a[0] 至 a[4] 加载到寄存器 a0 至 a5 中。
        "ld a0, %1\n"
        "ld a1, %2\n"
        "ld a2, %3\n"
        "ld a3, %4\n"
        "ld a4, %5\n"
    	"ld a5, %6\n"
        // 执行 ecall 指令来触发系统调用
        "ecall\n"
        // 将系统调用的返回值从寄存器 a0 存储到变量 ret。
        "sd a0, %0"
        : "=m" (ret)
        : "m"(num), "m"(a[0]), "m"(a[1]), "m"(a[2]), "m"(a[3]), "m"(a[4])
        :"memory");
        //num存到a0寄存器， a[0]存到a1寄存器
        //ecall的返回值存到ret
    return ret;
}

int
sys_exit(int64_t error_code) {
    return syscall(SYS_exit, error_code);
}

int
sys_fork(void) {
    return syscall(SYS_fork);
}

int
sys_wait(int64_t pid, int *store) {
    return syscall(SYS_wait, pid, store);
}

int
sys_yield(void) {
    return syscall(SYS_yield);
}

int
sys_kill(int64_t pid) {
    return syscall(SYS_kill, pid);
}

int
sys_getpid(void) {
    return syscall(SYS_getpid);
}

int
sys_putc(int64_t c) {
    return syscall(SYS_putc, c);
}

int
sys_pgdir(void) {
    return syscall(SYS_pgdir);
}


#include <defs.h>
#include <syscall.h>
#include <stdio.h>
#include <ulib.h>

void
exit(int error_code) {
    sys_exit(error_code);
    // 在用户程序里使用的cprintf()也是在user/libs/stdio.c重新实现的，
    // 和之前比最大的区别是，打印字符的时候需要经过系统调用sys_putc()，
    // 而不能直接调用sbi_console_putchar()。这是自然的，因为只有在
    // Supervisor Mode才能通过ecall调用Machine Mode的OpenSBI接口，
    // 而在用户态(U Mode)就不能直接使用M mode的接口，而是要通过系统调用。
    cprintf("BUG: exit failed.\n");
    while (1);
}

int
fork(void) {
    return sys_fork();
}

int
wait(void) {
    return sys_wait(0, NULL);
}

int
waitpid(int pid, int *store) {
    return sys_wait(pid, store);
}

void
yield(void) {
    sys_yield();
}

int
kill(int pid) {
    return sys_kill(pid);
}

int
getpid(void) {
    return sys_getpid();
}

//print_pgdir - print the PDT&PT
void
print_pgdir(void) {
    sys_pgdir();
}


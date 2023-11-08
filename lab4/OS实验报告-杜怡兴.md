## 实验报告

### 前置分析

#### 进程结构的基本理解:

![1aba6bf5f6b98350ed8c49d9dfa6c62](C:\Users\86157\Documents\WeChat Files\wxid_54bh66rc4gc222\FileStorage\Temp\1aba6bf5f6b98350ed8c49d9dfa6c62.jpg)

#### 分配pid

last_pid+1得到准备分配的pid，如果没有进程占用，返回。否则，last_pid+1，+2，+3直到第一个没有进程占用的pid

![054548dca7ad325b0aa019d6583f1e2](C:\Users\86157\Documents\WeChat Files\wxid_54bh66rc4gc222\FileStorage\Temp\054548dca7ad325b0aa019d6583f1e2.jpg)


#### 添加哈希链表:

![19ab9c48943a85bd06acef78b57c0bf](C:\Users\86157\Documents\WeChat Files\wxid_54bh66rc4gc222\FileStorage\Temp\19ab9c48943a85bd06acef78b57c0bf.jpg)

#### 查找哈希链表

![e1c3bc8573bc8a3dc4db075d45d05a4](C:\Users\86157\Documents\WeChat Files\wxid_54bh66rc4gc222\FileStorage\Temp\e1c3bc8573bc8a3dc4db075d45d05a4.jpg)

#### schedule

![cbe1c71019d24f36e9ecdfd4e9922b2](C:\Users\86157\Documents\WeChat Files\wxid_54bh66rc4gc222\FileStorage\Temp\cbe1c71019d24f36e9ecdfd4e9922b2.jpg)



##  练习 0：填写已有实验

本实验依赖实验 1/2/3。请把你做的实验 1/2/3 的代码填入本实验中代码中有“LAB1”，“LAB2”，“LAB3”的注释相应部分。




## 练习 1：分配并初始化一个进程控制块（需要编码）

`alloc_proc` 函数（位于 `kern/process/proc.c` 中）负责分配并返回一个新的 `struct proc_struct` 结构，用于存储新建立的内核线程的管理信息。ucore 需要对这个结构进行最基本的初始化，你需要完成这个初始化过程。

```c
// alloc_proc - alloc a proc_struct and init all fields of proc_struct
static struct proc_struct *
alloc_proc(void) {
    struct proc_struct *proc = kmalloc(sizeof(struct proc_struct));
    if (proc != NULL) {
    //LAB4:EXERCISE1 YOUR CODE
    /*
     * below fields in proc_struct need to be initialized
     *       enum proc_state state;                      // Process state
     *       int pid;                                    // Process ID
     *       int runs;                                   // the running times of Proces
     *       uintptr_t kstack;                           // Process kernel stack
     *       volatile bool need_resched;                 // bool value: need to be rescheduled to release CPU?
     *       struct proc_struct *parent;                 // the parent process
     *       struct mm_struct *mm;                       // Process's memory management field
     *       struct context context;                     // Switch here to run process
     *       struct trapframe *tf;                       // Trap frame for current interrupt
     *       uintptr_t cr3;                              // CR3 register: the base addr of Page Directroy Table(PDT)
     *       uint32_t flags;                             // Process flag
     *       char name[PROC_NAME_LEN + 1];               // Process name
     */
        memset(proc, 0, sizeof(struct proc_struct));

        proc->state = PROC_UNINIT;
        proc->pid = -1;
        proc->cr3 = boot_cr3;
    }
    return proc;
}
```



- 请在实验报告中简要说明你的设计实现过程。请回答如下问题：

![767e73116f3a4a63798b8456a6f5157](C:\Users\86157\Documents\WeChat Files\wxid_54bh66rc4gc222\FileStorage\Temp\767e73116f3a4a63798b8456a6f5157.jpg)

- 请说明 `proc_struct` 中 `struct context context` 和 `struct trapframe *tf` 成员变量的含义和在本实验中的作用是什么？（提示通过看代码和编程调试可以判断出来）

**context的作用:**

1. switch_to(&(prev->context), &(next->context));切换上下文的时候要保存旧的寄存器，加载新的寄存器，最后跳转到新的上下文。对应一个汇编代码【见后面】。

   ![image-20231108213206926](C:\Users\86157\AppData\Roaming\Typora\typora-user-images\image-20231108213206926.png)

**trapfram和context区别**
`trapframe` 用于在用户态和内核态之间切换的情况，而 `context` 用于在内核态中进行线程或进程的上下文切换。当内核需要挂起一个线程并恢复另一个线程的执行时，它会使用 `context` 结构来保存和恢复必要的信息。当用户态程序引发异常或进行系统调用，导致控制权交给内核时，会使用 `trapframe` 来保存和恢复用户态程序的状态。

## 练习 2：为新创建的内核线程分配资源（需要编码）

创建一个内核线程需要分配和设置好很多资源。`kernel_thread` 函数通过调用 `do_fork` 函数完成具体内核线程的创建工作。`do_kernel` 函数会调用 `alloc_proc` 函数来分配并初始化一个进程控制块，但 `alloc_proc` 只是找到了一小块内存用以记录进程的必要信息，并没有实际分配这些资源。ucore 一般通过 `do_fork` 实际创建新的内核线程。`do_fork` 的作用是，创建当前内核线程的一个副本，它们的执行上下文、代码、数据都一样，但是存储位置不同。因此，我们实际需要"fork"的东西就是 `stack` 和 `trapframe`。在这个过程中，需要给新内核线程分配资源，并且复制原进程的状态。你需要完成在 `kern/process/proc.c` 中的 `do_fork` 函数中的处理过程。
```c
int
do_fork(uint32_t clone_flags, uintptr_t stack, struct trapframe *tf) {
    int ret = -E_NO_FREE_PROC;
    struct proc_struct *proc;
    if (nr_process >= MAX_PROCESS) {
        goto fork_out;
    }
    ret = -E_NO_MEM;
    //LAB4:EXERCISE2 YOUR CODE
    /*
     * Some Useful MACROs, Functions and DEFINEs, you can use them in below implementation.
     * MACROs or Functions:
     *   alloc_proc:   create a proc struct and init fields (lab4:exercise1)
     *   setup_kstack: alloc pages with size KSTACKPAGE as process kernel stack
     *   copy_mm:      process "proc" duplicate OR share process "current"'s mm according clone_flags
     *                 if clone_flags & CLONE_VM, then "share" ; else "duplicate"
     *   copy_thread:  setup the trapframe on the  process's kernel stack top and
     *                 setup the kernel entry point and stack of process
     *   hash_proc:    add proc into proc hash_list
     *   get_pid:      alloc a unique pid for process
     *   wakeup_proc:  set proc->state = PROC_RUNNABLE
     * VARIABLES:
     *   proc_list:    the process set's list
     *   nr_process:   the number of process set
     */

    //    1. call alloc_proc to allocate a proc_struct
    //    2. call setup_kstack to allocate a kernel stack for child process
    //    3. call copy_mm to dup OR share mm according clone_flag
    //    4. call copy_thread to setup tf & context in proc_struct
    //    5. insert proc_struct into hash_list && proc_list
    //    6. call wakeup_proc to make the new child process RUNNABLE
    //    7. set ret vaule using child proc's pid
    proc = alloc_proc();

    if (proc == NULL) {
        goto fork_out;
    }

    proc->parent = current;

    if (setup_kstack(proc) != 0) {
        goto bad_fork_cleanup_kstack;
    }

    if (copy_mm(clone_flags, proc) != 0) {
        goto bad_fork_cleanup_proc;
    }

    copy_thread(proc, stack, tf);

    bool intr_flag;
    local_intr_save(intr_flag);
    
    proc->pid = get_pid();
    hash_proc(proc);
    list_add(&proc_list, &(proc->list_link));
    nr_process++;

    local_intr_restore(intr_flag);

    wakeup_proc(proc);

    ret = proc->pid;


fork_out:
    return ret;

bad_fork_cleanup_kstack:
    put_kstack(proc);
bad_fork_cleanup_proc:
    kfree(proc);
    goto fork_out;
}

```

它的大致执行步骤包括：

- 调用 `alloc_proc`，首先获得一块用户信息块。
- 为进程分配一个内核栈。

![image-20231108231245432](C:\Users\86157\AppData\Roaming\Typora\typora-user-images\image-20231108231245432.png)

copy_thread
![image-20231108232750685](C:\Users\86157\AppData\Roaming\Typora\typora-user-images\image-20231108232750685.png)



## 练习 3：编写 `proc_run` 函数（需要编码）

`proc_run` 用于将指定的进程切换到 CPU 上运行。它的大致执行步骤包括：

- 检查要切换的进程是否与当前正在运行的进程相同，如果相同则不需要切换。
- 禁用中断。你可以使用 `/kern/sync/sync.h` 中定义好的宏 `local_intr_save(x)` 和 `local_intr_restore(x)` 来实现关、开中断。
- 切换当前进程为要运行的进程。
- 切换页表，以便使用新进程的地址空间。`/libs/riscv.h` 中提供了 `lcr3(unsigned int cr3)` 函数
- 实现上下文切换。`/kern/process` 中已经预先编写好了 `switch.S`，其中定义了 `switch_to()` 函数。可实现两个进程的 context 切换。
- 允许中断。
- 

  ```c
  // proc_run - make process "proc" running on cpu
  // NOTE: before call switch_to, should load  base addr of "proc"'s new PDT
  void
  proc_run(struct proc_struct *proc) {
      if (proc != current) {
          // LAB4:EXERCISE3 YOUR CODE
          /*
          * Some Useful MACROs, Functions and DEFINEs, you can use them in below implementation.
          * MACROs or Functions:
          *   local_intr_save():        Disable interrupts
          *   local_intr_restore():     Enable Interrupts
          *   lcr3():                   Modify the value of CR3 register
          *   switch_to():              Context switching between two processes
          */
          bool intr_flag;
          local_intr_save(intr_flag);
  
          struct proc_struct *prev = current;
          struct proc_struct *next = proc;
  
          current = proc;
          lcr3(proc->cr3);
          switch_to(&(prev->context), &(next->context));
  
          local_intr_restore(intr_flag);
         
      }
  }
  ```

  

  ![15710a585f945b8f66a5d010e627820](C:\Users\86157\Documents\WeChat Files\wxid_54bh66rc4gc222\FileStorage\Temp\15710a585f945b8f66a5d010e627820.jpg)

其中还有两个细节.

1. 切换上下文的时候
   <img src="C:\Users\86157\AppData\Roaming\Typora\typora-user-images\image-20231107235610096.png" alt="image-20231107235610096" style="zoom: 50%;" />
   switch_to是一个宏，先把上一个进程pre的ra,sp,s0-s1寄存器全部存下来，然后把当前进程的ra,sp,s0-s1寄存器全部加载到cpu寄存器当中。
   最后return 的时候，由于`ra`已经被加载了新的值，就可以跳转到新的上下文

2. lcr3 将cr3(当前进程根目录的物理地址)去掉偏移，只保留物理页号，拼接mode构成stap(sptbr)
   ![image-20231108000011451](C:\Users\86157\AppData\Roaming\Typora\typora-user-images\image-20231108000011451.png)



请回答如下问题：

- 在本实验的执行过程中，创建且运行了几个内核线程？

两个,idle和init

![52c9ae5171bf83a9e74f4259b85a10f](C:\Users\86157\Documents\WeChat Files\wxid_54bh66rc4gc222\FileStorage\Temp\52c9ae5171bf83a9e74f4259b85a10f.jpg)

![1b2416c47c95915aa4d6e6d51a462b5](C:\Users\86157\Documents\WeChat Files\wxid_54bh66rc4gc222\FileStorage\Temp\1b2416c47c95915aa4d6e6d51a462b5.jpg)
完成代码编写后，编译并运行代码：`make qemu`
![image-20231108233617218](C:\Users\86157\AppData\Roaming\Typora\typora-user-images\image-20231108233617218.png)

如果可以得到如附录 A 所示的显示内容（仅供参考，不是标准答案输出），则基本正确。
![image-20231108233957501](C:\Users\86157\AppData\Roaming\Typora\typora-user-images\image-20231108233957501.png)

##  扩展练习 Challenge：

- 说明语句 `local_intr_save(intr_flag);....local_intr_restore(intr_flag);` 是如何实现开关中断的？

关键点是sstatus的SIE位,SIE=1表示全局中断使能,SIE=0表示禁用。

local_intr_save宏的工作流程是:

1. 读取sstatus寄存器,检查SIE位是否为1,即是否已开中断
2. 如果已开中断,则通过intr_disable()宏将SIE位置0,禁用中断
3. 并将原来的中断状态(SIE位的值)保存到参数intr_flag

local_intr_restore宏的工作流程是:

1. 检查传入的intr_flag是否为1
2. 如果为1,说明原来是开中断的,那么通过intr_enable()将SIE位置1,恢复中断
3. 如果为0,说明原来是关中断的,那么不需要恢复中断

这样通过保存和恢复SIE位来实现关中断/开中断的功能。
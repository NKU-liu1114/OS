#include <vmm.h>
#include <sync.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <error.h>
#include <pmm.h>
#include <riscv.h>
#include <swap.h>

/* 
  vmm design include two parts: mm_struct (mm) & vma_struct (vma)
  mm is the memory manager for the set of continuous virtual memory  
  area which have the same PDT. vma is a continuous virtual memory area.
  There a linear link list for vma & a redblack link list for vma in mm.
---------------
  mm related functions:
   golbal functions
     struct mm_struct * mm_create(void)
     void mm_destroy(struct mm_struct *mm)
     int do_pgfault(struct mm_struct *mm, uint_t error_code, uintptr_t addr)
--------------
  vma related functions:
   global functions
     struct vma_struct * vma_create (uintptr_t vm_start, uintptr_t vm_end,...)
     void insert_vma_struct(struct mm_struct *mm, struct vma_struct *vma)
     struct vma_struct * find_vma(struct mm_struct *mm, uintptr_t addr)
   local functions
     inline void check_vma_overlap(struct vma_struct *prev, struct vma_struct *next)
---------------
   check correctness functions
     void check_vmm(void);
     void check_vma_struct(void);
     void check_pgfault(void);
*/

// szx func : print_vma and print_mm
void print_vma(char *name, struct vma_struct *vma){
	cprintf("-- %s print_vma --\n", name);
	cprintf("   mm_struct: %p\n",vma->vm_mm);
	cprintf("   vm_start,vm_end: %x,%x\n",vma->vm_start,vma->vm_end);
	cprintf("   vm_flags: %x\n",vma->vm_flags);
	cprintf("   list_entry_t: %p\n",&vma->list_link);
}

void print_mm(char *name, struct mm_struct *mm){
	cprintf("-- %s print_mm --\n",name);
	cprintf("   mmap_list: %p\n",&mm->mmap_list);
	cprintf("   map_count: %d\n",mm->map_count);
	list_entry_t *list = &mm->mmap_list;
	for(int i=0;i<mm->map_count;i++){
		list = list_next(list);
		print_vma(name, le2vma(list,list_link));
	}
}

static void check_vmm(void);
static void check_vma_struct(void);
static void check_pgfault(void);

// mm_create -  alloc a mm_struct & initialize it.
struct mm_struct *
mm_create(void) {  //用于创建一个 struct mm_struct 类型的内存管理结构。
    struct mm_struct *mm = kmalloc(sizeof(struct mm_struct));//分配了足够的内存以存储一个 struct mm_struct 结构，并将其指针赋给 mm 变量。

    if (mm != NULL) { //检查内存分配是否成功。如果 mm 不为空（即内存分配成功），则进入条件语句的代码块。
        list_init(&(mm->mmap_list));  //初始化一个链表结构，用于存储虚拟内存区域（VMA）结构。
        mm->mmap_cache = NULL;
        mm->pgdir = NULL;//用于指向页目录的指针
        mm->map_count = 0;// VMA 结构的数量。

        if (swap_init_ok) swap_init_mm(mm);
        else mm->sm_priv = NULL;
    }
    return mm;
}

// vma_create - alloc a vma_struct & initialize it. (addr range: vm_start~vm_end)
struct vma_struct *
vma_create(uintptr_t vm_start, uintptr_t vm_end, uint_t vm_flags) {
    struct vma_struct *vma = kmalloc(sizeof(struct vma_struct));

    if (vma != NULL) {
        vma->vm_start = vm_start;
        vma->vm_end = vm_end;
        vma->vm_flags = vm_flags;
    }
    return vma;
}


// find_vma - find a vma  (vma->vm_start <= addr <= vma_vm_end)
struct vma_struct *
find_vma(struct mm_struct *mm, uintptr_t addr) {
    struct vma_struct *vma = NULL;
    if (mm != NULL) {
        vma = mm->mmap_cache;
        if (!(vma != NULL && vma->vm_start <= addr && vma->vm_end > addr)) {
                bool found = 0;
                list_entry_t *list = &(mm->mmap_list), *le = list;
                while ((le = list_next(le)) != list) {
                    vma = le2vma(le, list_link);
                    if (vma->vm_start<=addr && addr < vma->vm_end) {
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    vma = NULL;
                }
        }
        if (vma != NULL) {
            mm->mmap_cache = vma;
        }
    }
    return vma;
}


// check_vma_overlap - check if vma1 overlaps vma2 ?
static inline void
check_vma_overlap(struct vma_struct *prev, struct vma_struct *next) {
    assert(prev->vm_start < prev->vm_end);
    assert(prev->vm_end <= next->vm_start);
    assert(next->vm_start < next->vm_end);
}


// insert_vma_struct -insert vma in mm's list link
void
insert_vma_struct(struct mm_struct *mm, struct vma_struct *vma) {
    assert(vma->vm_start < vma->vm_end);
    list_entry_t *list = &(mm->mmap_list);
    list_entry_t *le_prev = list, *le_next;

        list_entry_t *le = list;
        while ((le = list_next(le)) != list) {
            struct vma_struct *mmap_prev = le2vma(le, list_link);
            if (mmap_prev->vm_start > vma->vm_start) {
                break;
            }
            le_prev = le;
        }

    le_next = list_next(le_prev);

    /* check overlap */
    if (le_prev != list) {
        check_vma_overlap(le2vma(le_prev, list_link), vma);
    }
    if (le_next != list) {
        check_vma_overlap(vma, le2vma(le_next, list_link));
    }

    vma->vm_mm = mm;
    list_add_after(le_prev, &(vma->list_link));

    mm->map_count ++;
}

// mm_destroy - free mm and mm internal fields
void
mm_destroy(struct mm_struct *mm) {

    list_entry_t *list = &(mm->mmap_list), *le;
    while ((le = list_next(list)) != list) {
        list_del(le);
        kfree(le2vma(le, list_link),sizeof(struct vma_struct));  //kfree vma        
    }
    kfree(mm, sizeof(struct mm_struct)); //kfree mm
    mm=NULL;
}

// vmm_init - initialize virtual memory management
//          - now just call check_vmm to check correctness of vmm
void
vmm_init(void) {
    check_vmm();
}

// check_vmm - check correctness of vmm
static void
check_vmm(void) {   //检查虚拟内存管理的正确性。
    size_t nr_free_pages_store = nr_free_pages();
    check_vma_struct();      //检查虚拟内存区域（VMA）的结构或状态。
    check_pgfault();   //检查页面错误异常的处理

    nr_free_pages_store--;	// szx : Sv39三级页表多占一个内存页，所以执行此操作
    assert(nr_free_pages_store == nr_free_pages());   //存储了当前空闲页的数量.存储的空闲页数与当前实际的空闲页数是否相等，以确保操作是否影响了空闲页的统计。

    cprintf("check_vmm() succeeded.\n");
}

static void
check_vma_struct(void) {//检查虚拟内存区域（VMA）结构的函数
    size_t nr_free_pages_store = nr_free_pages();//存储了空闲页的数量。

    struct mm_struct *mm = mm_create();//创建了一个内存管理结构
    assert(mm != NULL);

    int step1 = 10, step2 = step1 * 10;//step1 和 step2 是步长值，用于循环创建 VMA 结构
    //通过循环创建了多个 VMA 结构，每个结构有不同的起始地址和结束地址，并将它们插入到内存管理结构中。
    int i;
    for (i = step1; i >= 1; i --) {
        struct vma_struct *vma = vma_create(i * 5, i * 5 + 2, 0);
        assert(vma != NULL);
        insert_vma_struct(mm, vma);
    }

    for (i = step1 + 1; i <= step2; i ++) {
        struct vma_struct *vma = vma_create(i * 5, i * 5 + 2, 0);
        assert(vma != NULL);
        insert_vma_struct(mm, vma);
    }

    list_entry_t *le = list_next(&(mm->mmap_list));//获取了内存管理结构中 VMA 列表的第一个元素。

    for (i = 1; i <= step2; i ++) { //通过循环遍历 VMA 列表，确保每个 VMA 的起始地址和结束地址设置正确。
        assert(le != &(mm->mmap_list));
        struct vma_struct *mmap = le2vma(le, list_link);
        assert(mmap->vm_start == i * 5 && mmap->vm_end == i * 5 + 2);
        le = list_next(le);
    }

    for (i = 5; i <= 5 * step2; i +=5) {
        struct vma_struct *vma1 = find_vma(mm, i);//通过一系列的 find_vma() 函数调用来查找特定地址范围内的 VMA，并确保它们的起始和结束地址是正确的。同时，也确保某些地址范围外没有对应的 VMA。
        assert(vma1 != NULL);
        struct vma_struct *vma2 = find_vma(mm, i+1);
        assert(vma2 != NULL);
        struct vma_struct *vma3 = find_vma(mm, i+2);
        assert(vma3 == NULL);
        struct vma_struct *vma4 = find_vma(mm, i+3);
        assert(vma4 == NULL);
        struct vma_struct *vma5 = find_vma(mm, i+4);
        assert(vma5 == NULL);

        assert(vma1->vm_start == i  && vma1->vm_end == i  + 2);
        assert(vma2->vm_start == i  && vma2->vm_end == i  + 2);
    }

    for (i =4; i>=0; i--) {
        struct vma_struct *vma_below_5= find_vma(mm,i);
        if (vma_below_5 != NULL ) {//对于低于 5 的地址范围，确保 find_vma() 返回为空（即对应的 VMA 不存在）
           cprintf("vma_below_5: i %x, start %x, end %x\n",i, vma_below_5->vm_start, vma_below_5->vm_end); 
        }
        assert(vma_below_5 == NULL);
    }

    mm_destroy(mm);//销毁了创建的内存管理结构。

    assert(nr_free_pages_store == nr_free_pages());//确保了在执行过程中没有影响到空闲页的数量。

    cprintf("check_vma_struct() succeeded!\n");
}

struct mm_struct *check_mm_struct;

// check_pgfault - check correctness of pgfault handler
static void
check_pgfault(void) {//检查页面错误异常处理
	// char *name = "check_pgfault";
    size_t nr_free_pages_store = nr_free_pages(); //存储了空闲页面的数量

    check_mm_struct = mm_create();//创建了一个内存管理结构

    assert(check_mm_struct != NULL);
    struct mm_struct *mm = check_mm_struct;//将这个新创建的内存管理结构指定给 mm 变量。
    pde_t *pgdir = mm->pgdir = boot_pgdir;//将引导页目录 boot_pgdir 分配给了 pgdir。
    assert(pgdir[0] == 0);//它验证了一个索引为 0 的页目录项的值为 0。

    struct vma_struct *vma = vma_create(0, PTSIZE, VM_WRITE);//创建了一个 VMA 结构，代表了一个可写的内存区域。

    assert(vma != NULL);

    insert_vma_struct(mm, vma);//将这个 VMA 结构插入到内存管理结构中

    //接下来，代码对从地址 0x100 开始的一段内存进行写入和读取，以及对数据的计算和检查，确保读写过程正确。
    uintptr_t addr = 0x100;
    assert(find_vma(mm, addr) == vma);

    int i, sum = 0;
    for (i = 0; i < 100; i ++) {
        *(char *)(addr + i) = i;
        sum += i;
    }
    for (i = 0; i < 100; i ++) {
        sum -= *(char *)(addr + i);
    }
    assert(sum == 0);

    page_remove(pgdir, ROUNDDOWN(addr, PGSIZE));//从页表中移除特定地址对应的页面

    free_page(pde2page(pgdir[0]));//释放了页表中第一个页面的内存

    pgdir[0] = 0;//将页目录的第一个条目设置为0

    mm->pgdir = NULL;//清空了 mm 结构中的页目录指针
    mm_destroy(mm);

    check_mm_struct = NULL;
    nr_free_pages_store--;	// szx : Sv39第二级页表多占了一个内存页，所以执行此操作

    assert(nr_free_pages_store == nr_free_pages());

    cprintf("check_pgfault() succeeded!\n");
}
//page fault number
volatile unsigned int pgfault_num=0;

/* do_pgfault - interrupt handler to process the page fault execption
 * @mm         : the control struct for a set of vma using the same PDT
 * @error_code : the error code recorded in trapframe->tf_err which is setted by x86 hardware
 * @addr       : the addr which causes a memory access exception, (the contents of the CR2 register)
 *
 * CALL GRAPH: trap--> trap_dispatch-->pgfault_handler-->do_pgfault
 * The processor provides ucore's do_pgfault function with two items of information to aid in diagnosing
 * the exception and recovering from it.
 *   (1) The contents of the CR2 register. The processor loads the CR2 register with the
 *       32-bit linear address that generated the exception. The do_pgfault fun can
 *       use this address to locate the corresponding page directory and page-table
 *       entries.
 *   (2) An error code on the kernel stack. The error code for a page fault has a format different from
 *       that for other exceptions. The error code tells the exception handler three things:
 *         -- The P flag   (bit 0) indicates whether the exception was due to a not-present page (0)
 *            or to either an access rights violation or the use of a reserved bit (1).
 *         -- The W/R flag (bit 1) indicates whether the memory access that caused the exception
 *            was a read (0) or write (1).
 *         -- The U/S flag (bit 2) indicates whether the processor was executing at user mode (1)
 *            or supervisor mode (0) at the time of the exception.
 */
int//处理页面错误异常
do_pgfault(struct mm_struct *mm, uint_t error_code, uintptr_t addr) {//addr: 访问出错的虚拟地址
    int ret = -E_INVAL;
    //try to find a vma which include addr
    struct vma_struct *vma = find_vma(mm, addr); //通过调用 find_vma 函数在 mm_struct 中检查给定的虚拟地址 addr 是否在有效的虚拟内存区域（VMA）内。

    pgfault_num++;
    //If the addr is in the range of a mm's vma?
    if (vma == NULL || vma->vm_start > addr) {//如果地址不在任何VMA的范围内或VMA的起始地址大于要访问的地址，则输出错误信息并返回失败。
        cprintf("not valid addr %x, and  can not find it in vma\n", addr);
        goto failed;
    }

    /* IF (write an existed addr ) OR
     *    (write an non_existed addr && addr is writable) OR
     *    (read  an non_existed addr && addr is readable)
     * THEN
     *    continue process
     */
    uint32_t perm = PTE_U;
    if (vma->vm_flags & VM_WRITE) {//根据VMA的属性确定页面的权限。若VMA是可写的，就设置相应的权限标志。
        perm |= (PTE_R | PTE_W);//perm 设置为可读和可写标志位。
    }
    addr = ROUNDDOWN(addr, PGSIZE);

    ret = -E_NO_MEM;

    pte_t *ptep=NULL;
    /*
    * Maybe you want help comment, BELOW comments can help you finish the code
    *
    * Some Useful MACROs and DEFINEs, you can use them in below implementation.
    * MACROs or Functions:
    *   get_pte : get an pte and return the kernel virtual address of this pte for la
    *             if the PT contians this pte didn't exist, alloc a page for PT (notice the 3th parameter '1')
    *   pgdir_alloc_page : call alloc_page & page_insert functions to allocate a page size memory & setup
    *             an addr map pa<--->la with linear address la and the PDT pgdir
    * DEFINES:
    *   VM_WRITE  : If vma->vm_flags & VM_WRITE == 1/0, then the vma is writable/non writable
    *   PTE_W           0x002                   // page table/directory entry flags bit : Writeable
    *   PTE_U           0x004                   // page table/directory entry flags bit : User can access
    * VARIABLES:
    *   mm->pgdir : the PDT of these vma
    *
    */


    ptep = get_pte(mm->pgdir, addr, 1);  //(1) 通过 get_pte 函数获取页表项指针 ptep。若页表项不存在，则分配一个新的页表。
    if (*ptep == 0) {//页表项 ptep 为空（值为0），表示当前的虚拟地址未映射到任何物理页面。
        if (pgdir_alloc_page(mm->pgdir, addr, perm) == NULL) {
            cprintf("pgdir_alloc_page in do_pgfault failed\n");
            goto failed;
        }
    } else {//处理缺页情况
        /*LAB3 EXERCISE 3: YOUR CODE
        * 请你根据以下信息提示，补充函数
        * 现在我们认为pte是一个交换条目，那我们应该从磁盘加载数据并放到带有phy addr的页面，
        * 并将phy addr与逻辑addr映射，触发交换管理器记录该页面的访问情况
        *
        *  一些有用的宏和定义，可能会对你接下来代码的编写产生帮助(显然是有帮助的)
        *  宏或函数:
        *    swap_in(mm, addr, &page) : 分配一个内存页，然后根据
        *    PTE中的swap条目的addr，找到磁盘页的地址，将磁盘页的内容读入这个内存页
        *    page_insert ： 建立一个Page的phy addr与线性addr la的映射
        *    swap_map_swappable ： 设置页面可交换
        */
        if (swap_init_ok) {
            struct Page *page = NULL;
            // 你要编写的内容在这里，请基于上文说明以及下文的英文注释完成代码编写
            //(1）According to the mm AND addr, try
            //to load the content of right disk page
            //into the memory which page managed.
             // 将物理页换入到内存中
            ret = swap_in(mm, addr, &page);
            if (ret != 0) {
                cprintf("swap_in in do_pgfault failed\n");
                goto failed;
            }
            //(2) According to the mm,
            //addr AND page, setup the
            //map of phy addr <--->
            //logical addr
            // 将物理页与虚拟页建立映射关系.在内存管理结构 mm 的页表中，将这个物理页面和对应的虚拟地址 addr 建立映射关系。
            page_insert(mm->pgdir,page,addr,perm);
            //(3) make the page swappable.
            // 设置当前的物理页为可交换的.即标记这个物理页面可以被换出到磁盘中。
            swap_map_swappable(mm,addr,page,1);
            //记录页面地址信息：将这个内存页面的虚拟地址信息（pra_vaddr）设置为 addr。
            page->pra_vaddr = addr;
        } else {
            cprintf("no swap_init_ok but ptep is %x, failed\n", *ptep);
            goto failed;
        }
   }

   ret = 0;
failed:
    return ret;
}


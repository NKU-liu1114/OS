#include <pmm.h>
#include <list.h>
#include <string.h>
#include <default_pmm.h>

/* In the first fit algorithm, the allocator keeps a list of free blocks (known as the free list) and,
   on receiving a request for memory, scans along the list for the first block that is large enough to
   satisfy the request. If the chosen block is significantly larger than that requested, then it is 
   usually split, and the remainder added to the list as another free block.
   Please see Page 196~198, Section 8.2 of Yan Wei Min's chinese book "Data Structure -- C programming language"
*/
// you should rewrite functions: default_init,default_init_memmap,default_alloc_pages, default_free_pages.
/*
 * Details of FFMA
 * (1) Prepare: In order to implement the First-Fit Mem Alloc (FFMA), we should manage the free mem block use some list.
 *              The struct free_area_t is used for the management of free mem blocks. At first you should
 *              be familiar to the struct list in list.h. struct list is a simple doubly linked list implementation.
 *              You should know howto USE: list_init, list_add(list_add_after), list_add_before, list_del, list_next, list_prev
 *              Another tricky method is to transform a general list struct to a special struct (such as struct page):
 *              you can find some MACRO: le2page (in memlayout.h), (in future labs: le2vma (in vmm.h), le2proc (in proc.h),etc.)
 * (2) default_init: you can reuse the  demo default_init fun to init the free_list and set nr_free to 0.
 *              free_list is used to record the free mem blocks. nr_free is the total number for free mem blocks.
 * (3) default_init_memmap:  CALL GRAPH: kern_init --> pmm_init-->page_init-->init_memmap--> pmm_manager->init_memmap
 *              This fun is used to init a free block (with parameter: addr_base, page_number).
 *              First you should init each page (in memlayout.h) in this free block, include:
 *                  p->flags should be set bit PG_property (means this page is valid. In pmm_init fun (in pmm.c),
 *                  the bit PG_reserved is setted in p->flags)
 *                  if this page  is free and is not the first page of free block, p->property should be set to 0.
 *                  if this page  is free and is the first page of free block, p->property should be set to total num of block.
 *                  p->ref should be 0, because now p is free and no reference.
 *                  We can use p->page_link to link this page to free_list, (such as: list_add_before(&free_list, &(p->page_link)); )
 *              Finally, we should sum the number of free mem block: nr_free+=n
 * (4) default_alloc_pages: search find a first free block (block size >=n) in free list and reszie the free block, return the addr
 *              of malloced block.
 *              (4.1) So you should search freelist like this:
 *                       list_entry_t le = &free_list;
 *                       while((le=list_next(le)) != &free_list) {
 *                       ....
 *                 (4.1.1) In while loop, get the struct page and check the p->property (record the num of free block) >=n?
 *                       struct Page *p = le2page(le, page_link);
 *                       if(p->property >= n){ ...
 *                 (4.1.2) If we find this p, then it' means we find a free block(block size >=n), and the first n pages can be malloced.
 *                     Some flag bits of this page should be setted: PG_reserved =1, PG_property =0
 *                     unlink the pages from free_list
 *                     (4.1.2.1) If (p->property >n), we should re-caluclate number of the the rest of this free block,
 *                           (such as: le2page(le,page_link))->property = p->property - n;)
 *                 (4.1.3)  re-caluclate nr_free (number of the the rest of all free block)
 *                 (4.1.4)  return p
 *               (4.2) If we can not find a free block (block size >=n), then return NULL
 * (5) default_free_pages: relink the pages into  free list, maybe merge small free blocks into big free blocks.
 *               (5.1) according the base addr of withdrawed blocks, search free list, find the correct position
 *                     (from low to high addr), and insert the pages. (may use list_next, le2page, list_add_before)
 *               (5.2) reset the fields of pages, such as p->ref, p->flags (PageProperty)
 *               (5.3) try to merge low addr or high addr blocks. Notice: should change some pages's p->property correctly.
 */
// 空闲页表:用一个节点指针和空闲页数量组成
free_area_t free_area;

#define free_list (free_area.free_list)     // the list header
#define nr_free (free_area.nr_free)         // number of free pages in this free list，类似于一个全局变量

static void
default_init(void) {
    list_init(&free_list);
    nr_free = 0;                            // nr_free可以理解为在这里可以使用的一个全局变量，记录所有页块中可用的物理页数
}
//  实例：init_memmap(pa2page(mem_begin), (mem_end - mem_begin) / PGSIZE);
static void
default_init_memmap(struct Page *base, size_t n) {  // 初始化以地址base开始的n个页块
    assert(n > 0);                                  // 断言语句，如果n<=0则停止执行
    struct Page *p = base;
    for (; p != base + n; p ++) {                   // 遍历n个页块，设为可自由分配
        assert(PageReserved(p));                    // 断言是内核保留页
        p->flags = p->property = 0;                 // 每个页块初始化              
        set_page_ref(p, 0);                         // p->ref=0
    }
    base->property = n;                             // 有n个空闲页块
    SetPageProperty(base);                          // 将base->flag设为1，表示是空闲页块的开头
    nr_free += n;                                   // 总的空闲页数加n
    if (list_empty(&free_list)) {                   // 如果空闲页块链表是空的
        list_add(&free_list, &(base->page_link));   // 在链表后附加上当前空闲页块
    } else {                                        // 非空
        list_entry_t* le = &free_list;
        while ((le = list_next(le)) != &free_list) {    // 完成一次遍历，遍历终止条件即为回到空闲页块链表的表头（因为这是双向链表）
            struct Page* page = le2page(le, page_link); // 拿到每次遍历页块的结构体地址
            if (base < page) {
                list_add_before(le, &(base->page_link));// 将新的页块加在之前
                break;
            } else if (list_next(le) == &free_list) {   // 已经完成遍历，即base地址大于最后一个空闲页块地址，此时将新的页块附加在之后
                list_add(le, &(base->page_link));
            }
        }
    }
}

// 分配n个页
static struct Page *
default_alloc_pages(size_t n) {
    assert(n > 0);
    if (n > nr_free) {          // 需要的页大于总的空闲的页
        return NULL;
    }
    struct Page *page = NULL;
    list_entry_t *le = &free_list;
    while ((le = list_next(le)) != &free_list) {  // 遍历空闲页块链表
        struct Page *p = le2page(le, page_link);  // 拿到每次遍历页块的结构体地址
        if (p->property >= n) {                   // first_fit算法的核心：一旦满足需要的页，即进行分配
            page = p;                             // 将页块的首地址赋给返回的结果
            break;
        }
    }
    if (page != NULL) {                           // 即在上一步进行了赋值
        list_entry_t* prev = list_prev(&(page->page_link));  
        list_del(&(page->page_link));             // 将页块从freelist上摘下来
        if (page->property > n) {
            struct Page *p = page + n;            // 消耗前n个页
            p->property = page->property - n;     // 页块的空闲页数减少n
            SetPageProperty(p);                   // 将p设为页块头，进行页块的切割
            list_add(prev, &(p->page_link));      // 再将p开始的新页块重新放回空闲页块链表
        }
        nr_free -= n;                             // 全局的总空闲页块减少n
        ClearPageProperty(page);                  // 将page开始的旧页块重置
    }
    return page;                                  // 返回分配的n个页
}

static void
default_free_pages(struct Page *base, size_t n) {// 释放base开始的具有n个页的页块
    assert(n > 0);                               // 断言n>0
    struct Page *p = base;
    for (; p != base + n; p ++) {                // 遍历页块数组
        assert(!PageReserved(p) && !PageProperty(p));//不是保留页并且已被分配
        p->flags = 0;                            // 重置
        set_page_ref(p, 0);
    }
    base->property = n;                          // 该页块的空闲页数为n
    SetPageProperty(base);                       // 表示从base开始的空闲页块
    nr_free += n;                                // 总的空闲页数+n

    if (list_empty(&free_list)) {                // 空闲链表是空的
        list_add(&free_list, &(base->page_link));// 在链表后附加上当前空闲页块
    } else {
        list_entry_t* le = &free_list;
        while ((le = list_next(le)) != &free_list) {// 与初始化的时候类似，遍历空闲页块链表，将刚释放的空闲页块插入到链表中去
            struct Page* page = le2page(le, page_link);
            if (base < page) {                      
                list_add_before(le, &(base->page_link));
                break;
            } else if (list_next(le) == &free_list) {
                list_add(le, &(base->page_link));
            }
        }
    }
    // 考虑合并的问题
    list_entry_t* le = list_prev(&(base->page_link));// 前一个页块
    if (le != &free_list) {                          // 前一个页块不是空闲页块链表的首项
        p = le2page(le, page_link);                  
        if (p + p->property == base) {               // 链表上的前一个页块和base页块内存上紧邻，即链表上逻辑相邻且地址上物理相邻
            p->property += base->property;           // 进行合并
            ClearPageProperty(base);                 // 清除base的状态
            list_del(&(base->page_link));            // 从链表上摘下base页块，用p页块覆盖掉base页块
            base = p;
        }
    }

    le = list_next(&(base->page_link));              // 后一个页块，与上类似 
    if (le != &free_list) {
        p = le2page(le, page_link);
        if (base + base->property == p) {
            base->property += p->property;
            ClearPageProperty(p);
            list_del(&(p->page_link));
        }
    }
}

static size_t
default_nr_free_pages(void) {
    return nr_free;
}

static void
basic_check(void) {
    struct Page *p0, *p1, *p2;
    p0 = p1 = p2 = NULL;
    assert((p0 = alloc_page()) != NULL);
    assert((p1 = alloc_page()) != NULL);
    assert((p2 = alloc_page()) != NULL);

    assert(p0 != p1 && p0 != p2 && p1 != p2);
    assert(page_ref(p0) == 0 && page_ref(p1) == 0 && page_ref(p2) == 0);

    assert(page2pa(p0) < npage * PGSIZE);
    assert(page2pa(p1) < npage * PGSIZE);
    assert(page2pa(p2) < npage * PGSIZE);

    list_entry_t free_list_store = free_list;
    list_init(&free_list);
    assert(list_empty(&free_list));

    unsigned int nr_free_store = nr_free;
    nr_free = 0;

    assert(alloc_page() == NULL);

    free_page(p0);
    free_page(p1);
    free_page(p2);
    assert(nr_free == 3);

    assert((p0 = alloc_page()) != NULL);
    assert((p1 = alloc_page()) != NULL);
    assert((p2 = alloc_page()) != NULL);

    assert(alloc_page() == NULL);

    free_page(p0);
    assert(!list_empty(&free_list));

    struct Page *p;
    assert((p = alloc_page()) == p0);
    assert(alloc_page() == NULL);

    assert(nr_free == 0);
    free_list = free_list_store;
    nr_free = nr_free_store;

    free_page(p);
    free_page(p1);
    free_page(p2);
}

// LAB2: below code is used to check the first fit allocation algorithm
// NOTICE: You SHOULD NOT CHANGE basic_check, default_check functions!
static void
default_check(void) {
    int count = 0, total = 0;
    list_entry_t *le = &free_list;
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        assert(PageProperty(p));
        count ++, total += p->property;
    }
    assert(total == nr_free_pages());

    basic_check();

    struct Page *p0 = alloc_pages(5), *p1, *p2;
    assert(p0 != NULL);
    assert(!PageProperty(p0));

    list_entry_t free_list_store = free_list;
    list_init(&free_list);
    assert(list_empty(&free_list));
    assert(alloc_page() == NULL);

    unsigned int nr_free_store = nr_free;
    nr_free = 0;

    free_pages(p0 + 2, 3);
    assert(alloc_pages(4) == NULL);
    assert(PageProperty(p0 + 2) && p0[2].property == 3);
    assert((p1 = alloc_pages(3)) != NULL);
    assert(alloc_page() == NULL);
    assert(p0 + 2 == p1);

    p2 = p0 + 1;
    free_page(p0);
    free_pages(p1, 3);
    assert(PageProperty(p0) && p0->property == 1);
    assert(PageProperty(p1) && p1->property == 3);

    assert((p0 = alloc_page()) == p2 - 1);
    free_page(p0);
    assert((p0 = alloc_pages(2)) == p2 + 1);

    free_pages(p0, 2);
    free_page(p2);

    assert((p0 = alloc_pages(5)) != NULL);
    assert(alloc_page() == NULL);

    assert(nr_free == 0);
    nr_free = nr_free_store;

    free_list = free_list_store;
    free_pages(p0, 5);

    le = &free_list;
    while ((le = list_next(le)) != &free_list) {
        struct Page *p = le2page(le, page_link);
        count --, total -= p->property;
    }
    assert(count == 0);
    assert(total == 0);
}
//这个结构体在
const struct pmm_manager default_pmm_manager = {
    .name = "default_pmm_manager",
    .init = default_init,
    .init_memmap = default_init_memmap,
    .alloc_pages = default_alloc_pages,
    .free_pages = default_free_pages,
    .nr_free_pages = default_nr_free_pages,
    .check = default_check,
};


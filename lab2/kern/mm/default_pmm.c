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
free_area_t free_area;//和freelist相差不大

#define free_list (free_area.free_list)//free_area.free_list太长了，改称free_list,空闲页块链表
#define nr_free (free_area.nr_free)//空闲页的总数

static void
default_init(void) {
    list_init(&free_list);//链表首位互指
    nr_free = 0;//空闲页数=0
}

static void
default_init_memmap(struct Page *base, size_t n) {//从base这个页开始，将后面的n页(包含base)作为一个页块，设置为free状态，加入空闲页块链表
    assert(n > 0);//assert是一个宏，如果断言失败，终止执行.将0个以及以下的页作为一个页块显然不合理
    struct Page *p = base;//p是一个页指针，刚开始指向base
    for (; p != base + n; p ++) {//遍历了页面[base base+1 ... base+n-1 ] 总共有n个页面
        assert(PageReserved(p));//中间判断页面是否是保留的，初始化工作应该是对保留的进行，如果不是保留的，退出
        p->flags = p->property = 0;//所有页面开始的时候flags和property都是0
        set_page_ref(p, 0);//ref也是0
    }
    base->property = n;//只有第一个页面property是n，表示它是页块的第一个页
    SetPageProperty(base);//通过flags里面的property表示一个页是不是页块开头
    nr_free += n;//从base这个页开始，将后面的n页(包含base)作为一个页块，设置为free状态，因此free list里面的空闲页增加了n页
    if (list_empty(&free_list)) {//如果freelist是空的
        list_add(&free_list, &(base->page_link));//将base代表的页块直接加入property
    } else {
        list_entry_t* le = &free_list;//le list_entry链表节点指针，指向freelist链表的第一个节点
        while ((le = list_next(le)) != &free_list) {//当le的next没有指向结尾【由于freelist是一个循环链表，结尾是它自己】
            struct Page* page = le2page(le, page_link);//le2page是一个宏，从freelist链表节点，映射到对应页，这里相当于在遍历页
            if (base < page) {//如果base这个页地址小于page这个页的地址
                list_add_before(le, &(base->page_link));//此时把base页里面的链表节点放在page对应的链表节点的前面
                break;//结束
            } else if (list_next(le) == &free_list) {//如果到了结尾，相当于下一个链表节点是开头
                list_add(le, &(base->page_link));//此时把base插在最后面
            }
        }
    }
}

static struct Page *
default_alloc_pages(size_t n) {//需要n页的空闲空间，在空闲页块链表的所有页块里面，找一块至少有n页的，对它进行分配
    assert(n > 0);//n小于等于0没有意义
    if (n > nr_free) {//总的只nr_free页空闲页，需要n页的空闲页一定不行
        return NULL;
    }
    struct Page *page = NULL;
    list_entry_t *le = &free_list;//le指针指向链表的开头
    while ((le = list_next(le)) != &free_list) {//当没有结束时
        struct Page *p = le2page(le, page_link);//从链表指针变成页指针，相当于在遍历页块
        if (p->property >= n) {//如果当前页块的空闲页数大于等于n
            page = p;//此时确定用它。找到第一个就用，因此叫做first-fit
            break;
        }
    }
    if (page != NULL) {//如果找到了至少有n页空闲页的一个页块page
        list_entry_t* prev = list_prev(&(page->page_link));//暂时存储前面的页块，后面有用
        list_del(&(page->page_link));//暂时先把这个页块从空闲页块链表当中拿走
        if (page->property > n) {//如果这个页块不是刚好够【有n页空闲页，那样就不用放回来了】，那么还需要把剩下的放回来
            struct Page *p = page + n;//从page~page+n-1这些页都要拿走
            p->property = page->property - n;//第page+n页重新称为页块头部，后面跟着原来页块数-n个页块
            SetPageProperty(p);//设置这个页称为页块头部
            list_add(prev, &(p->page_link));//将这个新的页块插入链表
        }
        nr_free -= n;//由于分走了n个空闲页，所以空闲页总数-n
        ClearPageProperty(page);//page不再是页块的头部
    }
    return page;//返回找到的页块头部
}

static void
default_free_pages(struct Page *base, size_t n) {////从base这个页开始，将后面的n页(包含base)作为一个页块，设置为free状态，加入空闲页块链表，外加合并操作
    //这一段和init绝大部分是一样的，就不分别写了，区别在于下面
    assert(n > 0);
    struct Page *p = base;
    for (; p != base + n; p ++) {
        assert(!PageReserved(p) && !PageProperty(p));//init针对的所有页面都是被保留的，但是free针对的页面必须不是保留的，并且不是空闲页块的第一块
        p->flags = 0;//这里没有property=0操作
        set_page_ref(p, 0);
    }
    base->property = n;
    SetPageProperty(base);
    nr_free += n;

    if (list_empty(&free_list)) {
        list_add(&free_list, &(base->page_link));
    } else {
        list_entry_t* le = &free_list;
        while ((le = list_next(le)) != &free_list) {
            struct Page* page = le2page(le, page_link);
            if (base < page) {
                list_add_before(le, &(base->page_link));
                break;
            } else if (list_next(le) == &free_list) {
                list_add(le, &(base->page_link));
            }
        }
    }
    //==================================================================================================================
    list_entry_t* le = list_prev(&(base->page_link));//链表前一个
    if (le != &free_list) {//如果添加链表元素之前，链表是空的，添加之后，新元素的前一个元素就是free_list，如果前一个元素le不是free_list，说明之前链表不为空
        p = le2page(le, page_link);//找到le对应的页p
        if (p + p->property == base) {//如果页p开头的页块紧邻着base开头的页块，就可以合并，把base丢掉
            p->property += base->property;//p代表的页块里面的空闲页数增加了base页块的空闲页数 那么多
            ClearPageProperty(base);//base不再是空闲页块的开头
            list_del(&(base->page_link));//base对应的页块被从链表中移除
            base = p;//base暂时移动到p，因为可能还要向后合并
        }
    }

    le = list_next(&(base->page_link));//base下一个节点
    if (le != &free_list) {//不是哨兵节点
        p = le2page(le, page_link);//从链表节点找页块
        if (base + base->property == p) {//base页块恰好和这个页块相邻
            base->property += p->property;//base对应页块的容量+=后面页块的容量
            ClearPageProperty(p);//后面不再是页块
            list_del(&(p->page_link));//移除后面的页块
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


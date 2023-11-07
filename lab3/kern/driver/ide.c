#include <assert.h>
#include <defs.h>
#include <fs.h>
#include <ide.h>
#include <stdio.h>
#include <string.h>
#include <trap.h>
#include <riscv.h>

// 由于QEMU模拟没有磁盘，为了实现页面置换算法，在这里用内存模拟磁盘

void ide_init(void) {}
// 最多2块磁盘
#define MAX_IDE 2
// 一块磁盘最大的扇区数
#define MAX_DISK_NSECS 56
// SECTSIZE:512，模拟磁盘基本的数据单元(一个扇区)
static char ide[MAX_DISK_NSECS * SECTSIZE];

bool ide_device_valid(unsigned short ideno) { return ideno < MAX_IDE; }

size_t ide_device_size(unsigned short ideno) { return MAX_DISK_NSECS; }

// 读磁盘
int ide_read_secs(unsigned short ideno, uint32_t secno, void *dst,
                  size_t nsecs) {
    // ideno: 假设挂载了多块磁盘，选择哪一块磁盘 这里我们其实只有一块“磁盘”，这个参数就没用到
    int iobase = secno * SECTSIZE;
    // void *memcpy(void *dest, const void *src, size_t n);
    // dest：目标内存地址，也就是要将数据复制到的位置。
    // src：源内存地址，也就是要从哪里复制数据。
    // n：要复制的字节数，即要复制多少数据。
    memcpy(dst, &ide[iobase], nsecs * SECTSIZE);
    return 0;
}

// 写磁盘
int ide_write_secs(unsigned short ideno, uint32_t secno, const void *src,
                   size_t nsecs) {
    int iobase = secno * SECTSIZE;
    memcpy(&ide[iobase], src, nsecs * SECTSIZE);
    return 0;
}

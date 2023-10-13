#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "list.h"

void print_list(list_entry_t *list) {
    list_entry_t *itr = list_next(list); // 获取首个元素
    while (itr != list) {
        printf("%d -> ", itr->val);
        itr = list_next(itr);
    }
    printf("end\n");
}

int main() {
    // 创建一个头结点
    list_entry_t head;
    list_init(&head);

    // 确认链表为空
    if (list_empty(&head)) {
        printf("List is empty.\n");
    }

    // 添加几个元素到链表
    list_entry_t node1 = {.val = 1};
    list_entry_t node2 = {.val = 2};
    list_entry_t node3 = {.val = 3};

    list_add(&head, &node1);
    list_add(&head, &node2);
    list_add(&head, &node3);

    // 打印链表
    printf("List after adding elements: ");
    print_list(&head);

    // 从链表中删除一个元素
    list_del(&node2);

    // 再次打印链表
    printf("List after removing node2: ");
    print_list(&head);

    return 0;
}

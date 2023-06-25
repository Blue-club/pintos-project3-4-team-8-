#ifndef VM_ANON_H
#define VM_ANON_H
#include "vm/vm.h"
#include "include/devices/disk.h"
#include "vm/vm.h";
struct page;
enum vm_type;

struct anon_page {
    struct swap_anon *swap_anon;
};

struct swap_anon {
    struct page *n_page;
    int sector_index; // 섹터 시작 인덱스
    bool is_used;     // 사용 여부
    struct list_elem swap_elem;
};


void vm_anon_init (void);
bool anon_initializer (struct page *page, enum vm_type type, void *kva);

#endif

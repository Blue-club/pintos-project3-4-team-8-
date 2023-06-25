/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"
#include "threads/malloc.h"
#include "include/threads/vaddr.h"

struct list swap_slots;
static const size_t SECTOR_PER_PAGE = PGSIZE / DISK_SECTOR_SIZE;

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);


/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};

/* Initialize the data for anonymous pages */
void
vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
	
	// 디스크 공간 얻기
	swap_disk = disk_get(1, 1);
	// 스왑 리스트 초기화
	list_init(&swap_slots);  
	// 주어진 disk 사이즈
	size_t swap_size = disk_size(swap_disk) / SECTOR_PER_PAGE; 

	for (int i = 0; i < swap_size; i++) {
		struct swap_anon *swap_anon = malloc(sizeof(struct swap_anon));

		swap_anon->n_page = NULL;
		swap_anon->sector_index = (i * SECTOR_PER_PAGE);
		swap_anon->is_used = false;
		list_push_back(&swap_slots, &swap_anon->swap_elem);
	}
}

/* Initialize the file mapping */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &anon_ops;

	struct anon_page *anon_page = &page->anon;
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;
	struct swap_anon *now_anon = anon_page->swap_anon;


	for (int i = 0; i < SECTOR_PER_PAGE; i++) {
		disk_read(swap_disk, now_anon->sector_index + i, kva + (i * DISK_SECTOR_SIZE));
	}

	// printf("swap in 2\n\n");

	anon_page->swap_anon = NULL;
	now_anon->is_used = false;	
	

	// printf("swap in 3\n\n");

	return true;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
    struct anon_page *anon_page = &page->anon;
    struct swap_anon *now_swap = NULL;
    // 주어진 disk 사이즈
    size_t swap_size = disk_size(swap_disk) / SECTOR_PER_PAGE;
    
    // struct list_elem *temp_elem = list_front(&swap_slots);
    // now_swap = list_entry(temp_elem, struct swap_anon, swap_elem);

    // for (int i = 0; i < swap_size; i++) {
    //     if (!now_swap->is_used) {
    //         break;
    //     }
        
    //     temp_elem = list_next(temp_elem);
    //     now_swap = list_entry(temp_elem, struct swap_anon, swap_elem);
    // }

	struct list_elem* e = list_head (&swap_slots);

   	while ((e = list_next (e)) != list_end (&swap_slots)){
		struct swap_anon* swap_anon=list_entry(e, struct swap_anon, swap_elem);

		if(!swap_anon->is_used){
			now_swap = swap_anon;
		}
   	}

	if (now_swap == NULL) {
		return false;
	}

    // kva를 512바이트씩 여덟 번 나누어서 디스크에 기록
    for (int i = 0; i < 8; i++) {
        void *now_buf = (char *)page->frame->kva + (DISK_SECTOR_SIZE * i);
        
        disk_write(swap_disk, now_swap->sector_index + i, now_buf);
    }
    

    now_swap->is_used = true;
    anon_page->swap_anon = now_swap;
    page->frame = NULL;
    pml4_clear_page(thread_current()->pml4, page->va);
    
    return true;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
	struct anon_page *anon_page = &page->anon;
	
	free(page);
}

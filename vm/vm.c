/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "vm/inspect.h"
#include "include/threads/vaddr.h"
#include "threads/mmu.h"
#include <string.h>
#include <list.h>


/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
	list_init(&lru_list);
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable, vm_initializer *init, void *aux UNUSED) {

	ASSERT (VM_TYPE(type) != VM_UNINIT);

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new. */
		struct page *new_page = malloc(sizeof(struct page));
		void * now_init;

		switch (VM_TYPE(type)) {
			case VM_ANON:
				now_init = anon_initializer;
				break;
			case VM_FILE:
				now_init = file_backed_initializer;
				break;
			default:
				break;
		}

		uninit_new(new_page, upage, init, type, aux, now_init);

		new_page->writable = writable; 
		/* TODO: Insert the page into the spt. */
        if (!spt_insert_page(spt, new_page)) {
            free(new_page);
            goto err;
        }

		return true;
	}

err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */

/**
 * 인자로 주어진 spt에서 va에 해당하는 struct page를 찾는다.
 * 실패할 경우 NULL을 반환한다.
*/
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page *temp_page = malloc(sizeof(struct page));
	temp_page->va = pg_round_down(va);

	/* TODO: Fill this function. */
	struct hash_elem *e = hash_find(&spt->spth, &temp_page->h_elem);
	
	if (e != NULL) {
		struct page *found_page = hash_entry(e, struct page, h_elem);
		return found_page;
	}

	return NULL;
}

/* Insert PAGE into spt with validation. */
/**
 * 인자로 주어진 spt에 struct page를 삽입한다. 
 * 이 함수는 인자로 주어진 spt에 해당 가상 주소가 존재하는지 확인해야한다.
*/
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	/* TODO: Fill this function. */
	if (hash_insert(&spt->spth, &page->h_elem) == NULL) {
		succ = true;
	}

	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	struct list_elem *now_elem = list_head(&lru_list);
	 /* TODO: The policy for eviction is up to you. */
	while((now_elem = list_next(now_elem)) != list_tail(&lru_list)){
		victim = list_entry(now_elem, struct frame, frame_elem);
		return victim;
	}

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */
	list_remove(&victim->frame_elem);

	if(swap_out(victim->page)){

		return victim;
	}

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/

/**
 * palloc() 함수를 호출하여 페이지를 할당하고, 할당된 페이지의 프레임을 가져옵니다. 
 * 하지만 사용 가능한 페이지가 없을 경우, 페이지를 대체(evict)하여 사용 가능한 메모리 공간을 확보하고 그 페이지를 반환합니다. 
 * 이를 통해 항상 유효한 메모리 주소를 반환하며, 
 * 메모리가 가득 찬 경우에도 프레임을 대체(evict)하여 새로운 페이지 할당을 가능하게 합니다.
*/
static struct frame *
vm_get_frame (void) {
	// printf("page get frame\n\n");

	struct frame *frame = malloc(sizeof(struct frame));

	/* TODO: Fill this function. */
	frame->kva = palloc_get_multiple(PAL_USER, 1);

	if(frame->kva == NULL) {
		free(frame);		
		frame = vm_evict_frame();
	}

	frame->page = NULL;
	list_push_back(&lru_list, &frame->frame_elem);

	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
	struct thread *curr = thread_current();
	void *page_addr = pg_round_down(addr);

	while(spt_find_page(&curr->spt, page_addr) == NULL){
		vm_alloc_page_with_initializer(VM_ANON | VM_MARKER_0, page_addr, true, NULL, NULL);
		vm_claim_page(page_addr);

		page_addr += PGSIZE;
	}
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED, bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct thread *curr = thread_current();
	struct supplemental_page_table *spt UNUSED = &curr->spt;
	struct page *page;

	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	void *stack_bottom = (void *) (((uint8_t *) USER_STACK) - (PGSIZE * STACK_MAX_PAGES));
	void *stack_top = USER_STACK;

	page = spt_find_page(&spt->spth, addr);

	if(page == NULL && (addr >= stack_bottom && addr <= stack_top)) {
		if(addr < (f->rsp - (1 << 3)))
			return false;

		vm_stack_growth(addr);

		return true;
	}

	if (page == NULL) {
		return false;
	}

	if (!not_present) {
		return false;
	}

	return vm_do_claim_page(page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va) {
	struct thread *curr = thread_current();
	struct page *page = NULL;

	/* TODO: Fill this function */
	page = spt_find_page(&curr -> spt, va);

	if (page == NULL) {
		struct page *new_page = malloc(sizeof(struct page));

		if(new_page == NULL) {
			return false;
		}

		new_page->va = va;
		spt_insert_page(&curr -> spt, new_page);
	}

	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame();

	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	uint64_t n_pml4 = thread_current() -> pml4;
	bool succ = pml4_set_page(n_pml4, page->va, frame->kva, page->writable);

	if(!succ) {
		return false;
	}

	return swap_in (page, frame->kva);
}

uint64_t hash_func(const struct hash_elem *e, void *aux) {
	// 해당 요소의 페이지 찾기
	struct page *f_page = hash_entry(e, struct page, h_elem);

	// 페이지의 가상 주소를 해싱하여 해시 값을 반환
	return hash_bytes(&f_page->va, sizeof(f_page->va));
}

bool less_func (const struct hash_elem *a, const struct hash_elem *b, void *aux) {
	// 요소 a의 페이지
    struct page *page_a = hash_entry(a, struct page, h_elem);
	// 요소 b의 페이지
    struct page *page_b = hash_entry(b, struct page, h_elem);

	// 크기 순대로 정렬
	return page_a -> va < page_b -> va;
}

void destroy_func (struct hash_elem *e, void *aux) {
	struct thread *curr = thread_current();
	struct page *now_page = hash_entry(e, struct page, h_elem);

	if(VM_TYPE(now_page->operations->type) == VM_FILE && pml4_is_dirty(curr->pml4, now_page->va)) {
		file_seek(now_page->file.file_info->file, now_page->file.file_info->ofs);
		file_write(now_page->file.file_info->file, now_page->frame->kva, now_page->file.file_info->page_read_bytes);
		pml4_set_dirty(curr->pml4, now_page->va, 0);
	}

	destroy(now_page);
};

/* Initialize new supplemental page table */
/**
 * supplemental page table을 초기화한다. 
 * spt에 사용할 데이터 구조를 선택할 수 있다.
 * 새로운 프로세스가 시작할 때(initd) 그리고 프로세스가 포크될 때(__do_fork) 호출된다.
*/
void
supplemental_page_table_init (struct supplemental_page_table *spt) {
	// spt 테이블 -> 해시 테이블 사용.	
	hash_init(&spt->spth, hash_func, less_func, NULL);
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst, struct supplemental_page_table *src) {
	// 각각의 dst와 src가 NULL 포인터인지 확인한다.	
	if (dst == NULL || src == NULL) {
		return false;
	}

	// src 즉 spt의 엔트리를 순회하면서 각 페이지 정보를 dst에 복사하자.
	struct hash_iterator iterator; // 현재 페이지
	hash_first(&iterator, &src->spth);

	while(hash_next(&iterator)) {
		struct hash_elem *e = hash_cur(&iterator);
		struct page *now_page = hash_entry(e, struct page, h_elem);

		if(VM_TYPE(now_page->operations->type) == VM_UNINIT) {
			vm_alloc_page_with_initializer(VM_ANON, now_page->va, now_page->writable, now_page->uninit.init, now_page->uninit.aux);
			continue;
		}else {
			if (!vm_alloc_page_with_initializer(VM_TYPE(now_page->operations->type), now_page->va, now_page->writable, NULL, VM_TYPE(now_page->operations->type) == VM_FILE ? now_page->file.file_info : NULL)) {
				return false;
			}
		}
		
		if(!vm_claim_page(now_page->va)) {
			return false;
		}

		struct page *find_page = spt_find_page(dst, now_page->va);
		
		if(find_page == NULL) {
			return false;
		}

		memcpy(find_page->frame->kva, now_page->frame->kva, PGSIZE);	
	}


	return true;
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */

	// Clear the hash table using hash_clear
	hash_clear(&spt->spth, destroy_func);
}

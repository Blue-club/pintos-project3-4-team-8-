/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "include/userprog/process.h"
#include "filesys/file.h"
#include "threads/mmu.h"
#include <round.h>

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	enum vm_type now_type = type;

	page->operations = &file_ops;
	struct file_page *file_page = &page->file;
	file_page->file_type = now_type;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct thread *curr = thread_current();
	struct file_page *file_page UNUSED = &page->file;

	pml4_clear_page(curr->pml4, page->va);
	hash_delete(&curr->spt.spth, &page->h_elem);

	if(file_page->file_type & VM_MARKER_1) {
		// free(file_page->file_info->file);
		// free(page->file.file_info);
	}
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable, struct file *file, off_t offset) {
    void *now_addr = addr;
    size_t num_pages = DIV_ROUND_UP(length, PGSIZE);
	file = file_reopen(file);

	for (size_t i = 0; i < num_pages; i++) {
		size_t page_read_bytes = length < PGSIZE ? length : PGSIZE;
        size_t page_zero_bytes = PGSIZE - page_read_bytes;

        struct file_segment *now_file = malloc(sizeof(struct file_segment));

        now_file->file = malloc(sizeof(struct file));
		memcpy(now_file->file, file, sizeof(struct file));
        now_file->page_read_bytes = page_read_bytes;
        now_file->page_zero_bytes = page_zero_bytes;
        now_file->writable = writable;
		now_file->ofs = offset;

        file_seek(now_file->file, offset);


		if(i == num_pages - 1) {
			if(!vm_alloc_page_with_initializer(VM_FILE | VM_MARKER_1, now_addr, writable, lazy_load_segment, now_file)) {
				file_close(file);
				free(now_file);
				return NULL;
			}
		}else {
			if(!vm_alloc_page_with_initializer(VM_FILE , now_addr, writable, lazy_load_segment, now_file)) {
				file_close(file);
				free(now_file);
				return NULL;
			}
		}

        length -= page_read_bytes;
        offset += page_read_bytes;

        now_addr += PGSIZE;
	}

    return addr;
}

/* Do the munmap */
void
do_munmap (void *addr) {
	struct thread *curr = thread_current();
	void *now_addr = addr;
	struct page *now_page = spt_find_page(&curr->spt, now_addr);

	while(now_page != NULL && ((VM_TYPE(now_page->operations->type) == VM_FILE) || (VM_TYPE(now_page->operations->type) == VM_UNINIT))) {
		if(VM_TYPE(now_page->operations->type) == VM_FILE) {
			// 로드가 되어있는 파일. 

			// 파일 overwrite
			if(now_page->file.file_info->ofs >= 0){
				file_seek(now_page->file.file_info->file, now_page->file.file_info->ofs);
				file_write(now_page->file.file_info->file, now_addr, now_page->file.read_byte);
			}
			// off_t write_byte = file_write(now_page->file.file_info->file, now_addr, now_page->file.read_byte);

			if(now_page->file.file_type & VM_MARKER_1) {
				//마지막 페이지
				vm_dealloc_page(now_page);
				break;
			}
		}else {
			// 로드가 되지 않은 파일.

			if(now_page->uninit.type & VM_MARKER_1) {
				//마지막 페이지
				vm_dealloc_page(now_page);
				break;
			}
		}
		
		vm_dealloc_page(now_page);

		now_addr = addr + PGSIZE;
		now_page = spt_find_page(&curr->spt, now_addr);
	}
}
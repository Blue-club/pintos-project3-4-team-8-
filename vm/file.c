/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "include/userprog/process.h"
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
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
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
	struct file_page *file_page UNUSED = &page->file;

	// free(page);
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable, struct file *file, off_t offset) {
    void *now_addr = addr;
    size_t num_pages = DIV_ROUND_UP(length, PGSIZE);

	// printf("addr is %p\n\n", addr);

	while (length > 0) {
		size_t page_read_bytes = length < PGSIZE ? length : PGSIZE;
        size_t page_zero_bytes = PGSIZE - page_read_bytes;

        struct file_segment *now_file = malloc(sizeof(struct file_segment));

        now_file->file = malloc(sizeof(struct file));
		memcpy(now_file->file, file, sizeof(struct file));
        now_file->page_read_bytes = page_read_bytes;
        now_file->page_zero_bytes = page_zero_bytes;
        now_file->writable = writable;

        file_seek(now_file->file, offset);

        if(!vm_alloc_page_with_initializer(VM_FILE, now_addr, writable, lazy_load_segment, now_file)) {
			free(now_file);
			return NULL;
		}

        length -= page_read_bytes;
        offset += page_read_bytes;

        now_addr += PGSIZE;
	}

	// printf("mmap page va is : %p\n\n", addr);

    return addr;
}

/* Do the munmap */
void
do_munmap (void *addr) {
	struct thread *curr = thread_current();
	void *now_addr = addr;
	struct page *now_page = spt_find_page(&curr->spt, now_addr);

	printf("start now page is : %p\n\n" , now_page->va);
	printf("start now addr is : %p\n\n" , now_addr);

	while(now_page != NULL && (VM_TYPE(now_page->operations->type) == VM_FILE)) {
		file_write_at
		
		free(now_page->frame);
		free(now_page);


		printf("end now page is : %p\n\n" , now_page->va);
		printf("end now addr is : %p\n\n" , now_addr);

		now_addr = addr + PGSIZE;
	}
}
#ifndef VM_FILE_H
#define VM_FILE_H
#include "filesys/file.h"
#include "vm/vm.h"

struct page;
enum vm_type;

struct file_segment {
	struct file *file;
	size_t page_read_bytes;
	size_t page_zero_bytes;
	bool writable;
	off_t ofs;
};

struct file_page {
	struct file_segment *file_info;
	off_t read_byte;
	enum vm_type file_type;
};

void vm_file_init (void);
bool file_backed_initializer (struct page *page, enum vm_type type, void *kva);
void *do_mmap(void *addr, size_t length, int writable,
		struct file *file, off_t offset);
void do_munmap (void *va);
#endif

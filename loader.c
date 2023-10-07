/*
 * Loader Implementation
 *
 * 2022, Operating Systems
 * Vladimir Marin 322CAa
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>

#include "exec_parser.h"

#define MIN(a,b)  (((a) < (b)) ? (a) : (b))

static so_exec_t *exec;
static void *old_handler;
static int exec_fd;

int normaliser1(int x)
{
	if (x < 0)
		return -1;
	
	if (x == 0)
		return 0;
	
	return x;
}

int read_buff(int fd, void *buffer, int count)
{
	/* No. of bytes read */
	int bytes = 0;

	while (count > bytes) {
		int new_bytes = (int)read(fd, buffer + bytes, count - bytes);

		switch (normaliser1(new_bytes)) {
			default:
				bytes = bytes + new_bytes;
				break;
			
			/* Input Error*/
			case -1:
				return -1;
				break;

			/* End of File Reached */
			case 0:
				return bytes;
				break;	
		}
	}

	return bytes;
}

void set_permissions(so_seg_t *segment, void* mapped_addr)
{
	int permissions;
	permissions = 0;

	if(segment->perm & PERM_R)
		permissions += 1;

	if(segment->perm & PERM_W)
		permissions += 2;

	if(segment->perm & PERM_X)
		permissions += 4;

	mprotect(mapped_addr, getpagesize(), permissions);

}

void copy_in__page(so_seg_t *segment, uintptr_t offset, void *pageAddress)
{
	char *buffer = calloc(getpagesize(), sizeof(char));
	lseek(exec_fd, segment->offset + offset, SEEK_SET);

	int sum = (int)offset + getpagesize();

	if ((int)segment->file_size - sum >= 0)
	{
		read_buff(exec_fd, buffer, getpagesize());
		memcpy(pageAddress, buffer, getpagesize());
	}
	else
	{
		read_buff(exec_fd, buffer, segment->file_size - offset);
		memcpy(pageAddress, buffer, getpagesize());
	}
	free(buffer);
}



void so_map_page(uintptr_t page_fault, so_seg_t *segment)
{
	if (!segment->data) {
		int max_pages = segment->mem_size / getpagesize();
		segment->data = (void *)calloc(max_pages, sizeof(char));
	}

	/* Getting page start address */

	int index;
	uintptr_t offset;
	
	index = (page_fault - segment->vaddr) / getpagesize();
	
	if (((char *)(segment->data))[index] == 1) {
		fprintf(stderr, "Invalid permissions\n");
		signal(SIGSEGV, old_handler);
		raise(SIGSEGV);
	}

	offset = index * getpagesize();
	
	/* Mapping page */

	void *mapped_addr = mmap((char *)segment->vaddr + offset, getpagesize(),
							 PERM_R | PERM_W, MAP_FIXED | MAP_SHARED |
							 MAP_ANONYMOUS, -1, 0);

	if (mapped_addr == MAP_FAILED) {
		fprintf(stderr, "Mapping error.\n");
		exit(EXIT_FAILURE);
	}

	((char *)(segment->data))[index] = 1;

	/* Copying from file into memory */

	copy_in__page(segment, (int)offset, mapped_addr);

	/* Setting page permissions */

	mprotect(mapped_addr, getpagesize(), PROT_NONE);
	set_permissions(segment, mapped_addr);
}

int check_segment(uintptr_t fault_add, int i)
{
	if (fault_add >= exec->segments[i].vaddr)
		if (fault_add < exec->segments[i].vaddr +
			exec->segments[i].mem_size) {
				return 1;
			}

	return 0;
}

void segv_handler(int sig_no, siginfo_t *sig_info, void *context)
{
	if (!sig_info)
		exit(EXIT_FAILURE);

	uintptr_t fault_addr = (int)sig_info->si_addr;

	for (int i = 0; i < exec->segments_no; i++) {
		if(check_segment(fault_addr, i)) {
			so_map_page(fault_addr, &exec->segments[i]);
			return;
		}
	}

	fprintf(stderr, "SEGFAULT out of exec segments bounds.\n");
	signal(SIGSEGV, old_handler);
	raise(SIGSEGV);
}

int so_init_loader(void)
{
	int rc;

	/* Default handler */
	old_handler = signal(SIGSEGV, NULL);
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = segv_handler;
	sa.sa_flags = SA_SIGINFO;
	rc = sigaction(SIGSEGV, &sa, NULL);
	if (rc < 0) {
		perror("sigaction");
		return -1;
	}
	return 0;
}

int so_execute(char *path, char *argv[])
{
	exec = so_parse_exec(path);
	exec_fd = open(path, O_RDONLY, 0644);
	if (!exec)
		return -1;

	so_start_exec(exec, argv);

	return -1;
}

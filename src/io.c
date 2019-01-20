/*
 * Brian Chrzanowski
 * Sun Jan 20, 2019 14:46
 *
 * Input-Output Subsystem
 */

#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "io.h"

size_t mapping_len;

/* mmap_wrap : wrap mmap to avoid passing our program's needs */
void *io_mmap(int fd, size_t size)
{
	void *ptr;

	ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	if (!ptr) {
		/* errno_error */
	}

	mapping_len = size;

	return ptr;
}

/* munmap_wrap : wrap munmap to avoid passing our program's needs */
int io_munmap(void *ptr)
{
	int rc;

	rc = munmap(ptr, mapping_len);

	if (!rc) {
		/* errno_error; */
	}

	return rc;
}

int io_open(char *name, int flags)
{
	int rc;

	rc = open(name, O_RDWR | O_CREAT, flags);

	if (rc < 0) {
		/* errno_error */
	}

	return rc;
}

int io_resize(int fd, size_t size)
{
	int rc;

	rc = posix_fallocate(fd, 0, size);

	if (rc < 0) {
		/* errorno error here */
	}

	return rc;
}

int io_close(int fd)
{
	int rc;

	rc = close(fd);

	if (rc < 0) {
		/* errno_error */
	}

	return rc;
}


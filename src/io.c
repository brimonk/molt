/*
 * Brian Chrzanowski
 * Sun Jan 20, 2019 14:46
 *
 * Input-Output Subsystem
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "io.h"
#include "common.h"

size_t mapping_len;

static void io_errnohandle()
{
	char buf[128];

	memset(buf, 0, 128);

	strerror_r(errno, buf, sizeof(buf));
	fprintf(stderr, "%s\n", buf);
}

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

int io_open(char *name)
{
	int rc;

	rc = open(name, O_RDWR | O_CREAT, 0666);

	if (rc < 0) {
		/* errno_error */
		io_errnohandle();
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

/* lump functions */

/* io_lumpcheck : 0 if file has correct magic, else if it doesn't */
int io_lumpcheck(void *ptr)
{
	struct lump_header_t *hdr;

	hdr = ptr;
	if (hdr->magic != MOLTLUMP_MAGIC) {
		hdr->magic = MOLTLUMP_MAGIC;
		hdr->version = MOLTCURRVERSION;
		hdr->type = MOLTLUMP_TYPEBIO;

		return 1;
	}

	return 0;
}

int io_lumpsetup(void *ptr)
{
	/*
	 * just an example, store three lump_pot_ts, then store a matrix
	 */

	struct lump_header_t *hdr;

	struct lump_pos_t *pos;
	struct lump_mat_t *mat;

	int i;

	hdr = ptr;

	/* setup and handle the */
	hdr->lump[0].offset = sizeof(struct lump_header_t) + 1;
	hdr->lump[0].size = sizeof(struct lump_pos_t) * 3;

	pos = io_lumpgetid(ptr, MOLTLUMP_POSITIONS);

	for (i = 0; i < 3; i++) {
		pos[i].x = i * 3 + i;
		pos[i].y = i * 3 - i;
		pos[i].z = i * 3 * i;
	}

	return 0;
}

/* io_lumpgetid : gets a pointer to the lump via enum */
void *io_lumpgetid(void *ptr, int idx)
{
	struct lump_header_t *hdr;

	hdr = ptr;

	return (void *)(((char *)ptr) + hdr->lump[idx].offset);
}

/* io_lumprecnum : returns the number of records that lump has */
int io_lumprecnum(void *ptr, int idx)
{
	struct lump_header_t *hdr;
	size_t val;

	hdr = ptr;

	switch (idx) {
		case MOLTLUMP_POSITIONS:
			val = sizeof(struct lump_pos_t);
			break;

		case MOLTLUMP_MATRIX:
			val = sizeof(struct lump_mat_t);
			break;
	}

	return hdr->lump[idx].size / val;
}


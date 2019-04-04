/*
 * Brian Chrzanowski
 * Sun Jan 20, 2019 14:46
 *
 * Input-Output Subsystem
 *
 * wrapper functions handle errors within them and die if needed
 *
 * Functions
 * io_mmap   : mmap wrapper. sizes disk file to be mmapped 
 * io_munmap : munmap wrapper.
 * io_msync  : msync wrapper. flushes changes made.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "io.h"
#include "common.h"

#define MMAP_FLAGS MAP_SHARED

static size_t mapping_len;
char io_filename[256];

/* function declarations */
static int io_getpagesize();
static void io_errnohandle();
static int io_msync(void *base, void *ptr, size_t len, int flags);

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

void *io_mremap(void *oldaddr, size_t oldsize, size_t newsize)
{
	void *ret;

	ret = mremap(oldaddr, oldsize, newsize, MMAP_FLAGS);

	if (ret == MAP_FAILED) {
		io_errnohandle();
	}

	return ret;
}

/* io_mssync : synchronous msync */
int io_mssync(void *base, void *ptr, size_t len)
{
	return io_msync(base, ptr, len, MS_SYNC);
}

/* io_mssync : asynchronous msync */
int io_masync(void *base, void *ptr, size_t len)
{
	return io_msync(base, ptr, len, MS_ASYNC);
}

/* io_msync : msync wrapper - ensures we write with a PAGESIZE multiple */
static int io_msync(void *base, void *ptr, size_t len, int flags)
{
	unsigned long addlsize;
	int ret;

	addlsize = ((unsigned long)ptr) % io_getpagesize();

	// sync the entire page
	ret = msync(((char *)ptr) - addlsize, len + addlsize, flags);

	if (ret) {
		io_errnohandle();
	}

	return ret;
}

/* io_getpagesize : wrapper for os independence */
static int io_getpagesize()
{
	return getpagesize();
}

/* munmap_wrap : wrap munmap to avoid passing our program's needs */
int io_munmap(void *ptr)
{
	int rc;

	rc = munmap(ptr, mapping_len);

	if (rc < 0) {
		io_errnohandle();
	}

	return rc;
}

int io_open(char *name)
{
	int rc;

	rc = open(name, O_RDWR | O_CREAT, 0666);

	if (rc < 0) {
		io_errnohandle();
	}

	strncpy(io_filename, name, sizeof(io_filename));

	return rc;
}

char *io_getfilename()
{
	return &io_filename[0];
}

int io_resize(int fd, size_t size)
{
	int rc;

	rc = posix_fallocate(fd, 0, size);

	if (rc < 0) {
		io_errnohandle();
	}

	return rc;
}

int io_close(int fd)
{
	int rc;

	rc = close(fd);

	if (rc < 0) {
		io_errnohandle();
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

/* io_lumpgetbase : retrieves the base pointer for a lump */
void *io_lumpgetbase(void *base, int lumpid)
{
	struct lump_header_t *hdr;

	hdr = base;

	return (void *)(((char *)base) + hdr->lump[lumpid].offset);
}

/* io_lumpgetid : gets a pointer to an array in the lump */
void *io_lumpgetidx(void *base, int lumpid, int idx)
{
	void *ret;
	struct lump_header_t *hdr;
	int lumptotal;

	hdr = base;

	lumptotal = io_lumprecnum(base, lumpid);

	if (idx < lumptotal) { // valid according to the lump table
		ret = ((char *)base) + hdr->lump[lumpid].offset;
		ret = ((char *)ret) + hdr->lump[lumpid].elemsize * idx;
	} else {
		ret = NULL;
	}

	return ret;
}

/* io_lumprecnum : returns the number of records that a given lump has */
int io_lumprecnum(void *base, int lumpid)
{
	struct lump_header_t *hdr;

	hdr = base;

	return hdr->lump[lumpid].lumpsize / hdr->lump[lumpid].elemsize;
}

/* simple console message wrapper */

/* io_fprintf : platform agnostic printf */
int io_fprintf(FILE *fp, const char *fmt, ...)
{
	va_list plist;
	int ret;

	va_start(plist, fmt);
	ret = vfprintf(fp, fmt, plist);
	va_end(plist);

	return ret;
}


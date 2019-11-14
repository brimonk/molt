/*
 * Brian Chrzanowski
 * Mon Nov 04, 2019 13:03
 *
 * Linux System Interface
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <dlfcn.h>

#define MMAP_FLAGS MAP_SHARED

#include "common.h"
#include "sys.h"

// module globals
static size_t sys_mappinglen;
char sys_filename[256];

static void sys_errorhandle()
{
	fprintf(stderr, "%s", strerror(errno));
}

/* sys_mmap : system wrapper for mmap */
void *sys_mmap(int fd, size_t size)
{
	void *p;

	p = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);

	if (p == MAP_FAILED) {
		sys_errorhandle();
	}

	sys_mappinglen = size;

	return p;
}

/* sys_mmsync : system wrapper for mssync (sync memory mapped page) */
int sys_mssync(void *base, void *ptr, size_t len)
{
	return sys_msync(base, ptr, len, MS_SYNC);
}

/* sys_mmsync : system wrapper for mssync (async sync memory mapped page) */
int sys_masync(void *base, void *ptr, size_t len)
{
	return sys_msync(base, ptr, len, MS_ASYNC);
}

/* sys_msync : msync wrapper - ensures we write with a PAGESIZE multiple */
int sys_msync(void *base, void *ptr, size_t len, int flags)
{
	u64 addlsize;
	int rc;

	addlsize = ((unsigned long)ptr) % sys_getpagesize();

	rc = msync(((char *)ptr) - addlsize, len + addlsize, flags);

	if (rc) {
		sys_errorhandle();
	}

	return rc;
}

/* sys_munmap : system wrapper for munmap */
int sys_munmap(void *ptr)
{
	int rc;

	rc = munmap(ptr, sys_mappinglen);

	if (rc < 0) {
		sys_errorhandle();
	}

	return rc;
}

/* sys_getpagesize : system wrapper for getpagesize */
int sys_getpagesize()
{
	return getpagesize();
}

/* sys_open : system wrapper for open */
int sys_open(char *name)
{
	int rc;

	rc = open(name, O_RDWR|O_CREAT, 0666);

	if (rc < 0) {
		sys_errorhandle();
	} else {
		strncpy(sys_filename, name, sizeof(sys_filename));
	}

	return rc;
}

/* sys_open : system wrapper for close */
int sys_close(int fd)
{
	int rc;

	rc = close(fd);

	if (rc < 0) {
		sys_errorhandle();
	}

	return rc;
}

/* sys_resize : system wrapper for resizing a file */
int sys_resize(int fd, size_t size)
{
	int rc;

	rc = posix_fallocate(fd, 0, size);
	if (rc < 0) {
		sys_errorhandle();
	}

	return rc;
}

/* sys_resize : system wrapper for getting the size of a file */
u64 sys_getsize()
{
	struct stat st;
	stat(sys_filename, &st);
	return st.st_size;
}

/* sys_exists : system wrapper to see if a file currently exists */
int sys_exists(char *path)
{
	return access(path, F_OK) == 0;
}

/* sys_readfile : reads an entire file into a memory buffer */
char *sys_readfile(char *path)
{
	FILE *fp;
	s64 size;
	char *buf;

	fp = fopen(path, "r");
	if (!fp) {
		return NULL;
	}

	// get the file's size
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	buf = malloc(size + 1);
	memset(buf, 0, size + 1);

	fread(buf, 1, size, fp);
	fclose(fp);

	return buf;
}

/* sys_lumpcheck : 0 if file has correct magic, else if it doesn't */
int sys_lumpcheck(void *ptr)
{
	struct lump_header_t *hdr;

	hdr = ptr;
	if (hdr->meta.magic != MOLTLUMP_MAGIC) {
		hdr->meta.magic = MOLTLUMP_MAGIC;
		hdr->meta.version = MOLTCURRVERSION;
		hdr->meta.type = MOLTLUMP_TYPEBIO;

		return 1;
	}

	return 0;
}

/* sys_lumprecnum : returns the number of records a given lump has */
int sys_lumprecnum(void *base, int lumpid)
{
	struct lump_header_t *hdr;

	hdr = base;

	return hdr->lump[lumpid].lumpsize / hdr->lump[lumpid].elemsize;
}

/* sys_lumpgetbase : retrieves the base pointer for a lump */
void *sys_lumpgetbase(void *base, int lumpid)
{
	struct lump_header_t *hdr;

	hdr = base;

	return (void *)(((char *)base) + hdr->lump[lumpid].offset);
}

/* sys_lumpgetidx : gets a pointer to an array in the lump */
void *sys_lumpgetidx(void *base, int lumpid, int idx)
{
	void *ret;
	struct lump_header_t *hdr;
	int lumptotal;

	hdr = base;

	lumptotal = sys_lumprecnum(base, lumpid);

	if (idx < lumptotal) { // valid according to the lump table
		ret = ((char *)base) + hdr->lump[lumpid].offset;
		ret = ((char *)ret) + hdr->lump[lumpid].elemsize * idx;
	} else {
		ret = NULL;
	}

	return ret;
}

/* sys_lumpgetmeta : returns a pointer to a lumpmeta_t for a given lumpid */
struct lumpmeta_t *sys_lumpgetmeta(void *base, int lumpid)
{
	struct lump_header_t *hdr;

	hdr = base;

	return (struct lumpmeta_t *)(((char *)base) + hdr->lump[lumpid].offset);
}

/* sys_libopen : sys wrapper for dlopen */
void *sys_libopen(char *filename)
{
	void *p;

	p = dlopen(filename, RTLD_LAZY);
	if (!p) {
		fprintf(stderr, "sys_libopen error : %s\n", dlerror());
	}

	return p;
}

/* sys_libsym : sys wrapper for dlsym */
void *sys_libsym(void *handle, char *symbol)
{
	void *p;

	p = dlsym(handle, symbol);
	if (!p) {
		fprintf(stderr, "sys_dlsym error: %s\n", dlerror());
	}

	return p;
}

/* sys_libclose : sys wrapper for dlclose */
int sys_libclose(void *handle)
{
	int rc;

	rc = dlclose(handle);
	if (rc != 0) {
		fprintf(stderr, "sys_libclose error : %s\n", dlerror());
	}

	return rc;
}


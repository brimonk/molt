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
 *
 * TODO
 * 1. Get the IO functions working in Windows
 * 2. Determine if we need sync/async mapping syncing in windows, change the
 *    MS_SYNC and MS_ASYNC if you do
 * 3. Get io_mremap working on windows
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "io.h"
#include "common.h"

#if !_WIN32 // LINUX MODE
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#define MMAP_FLAGS MAP_SHARED
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys\types.h>
#include <io.h>

void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *start, size_t length);

/* macro definitions extracted from git/git-compat-util.h */
#define PROT_READ  1
#define PROT_WRITE 2

/* macro definitions extracted from /usr/include/bits/mman.h */
#define MAP_SHARED	0x01		/* Share changes.  */
#define MAP_PRIVATE	0x02		/* Changes are private.  */

// TODO do we actually need different types of syncing
#define MS_SYNC  0x01
#define MS_ASYNC 0x02

#endif

#define MAP_FAILED ((void*)-1)

static size_t mapping_len;
char io_filename[256];

/* function declarations */
static int io_getpagesize();
static void io_errnohandle();
static int io_msync(void *base, void *ptr, size_t len, int flags);

/* io_errnohandle : nicely gets the errno message and prints it */
static void io_errnohandle()
{
	fprintf(stderr, "%s", strerror(errno));
}

#ifdef _WIN32
/* mmap : windows wrapper for mmap */
void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
	HANDLE hmap;
	void *temp;
	size_t len;
	struct stat st;
	uint64_t o = offset;
	uint32_t l = o & 0xFFFFFFFF;
	uint32_t h = (o >> 32) & 0xFFFFFFFF;

	if (!fstat(fd, &st)) {
		len = (size_t) st.st_size;
	} else {
		fprintf(stderr,"mmap: could not determine filesize");
		exit(1);
	}

	if ((length + offset) > len)
		length = len - offset;

	if (!(flags & MAP_PRIVATE)) {
		fprintf(stderr,"Invalid usage of mmap when built with USE_WIN32_MMAP");
		exit(1);
	}

	hmap = CreateFileMapping((HANDLE)_get_osfhandle(fd), 0, PAGE_WRITECOPY, 0, 0, 0);

	if (!hmap)
		return MAP_FAILED;

	temp = MapViewOfFileEx(hmap, FILE_MAP_COPY, h, l, length, start);

	if (!CloseHandle(hmap))
		fprintf(stderr,"unable to close file mapping handle\n");

	return temp ? temp : MAP_FAILED;
}

#endif

/* io_mmap : wrap mmap so our program can */
void *io_mmap(int fd, size_t size)
{
	void *ptr;

	// ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);

	if (ptr == MAP_FAILED) {
		io_errnohandle();
	}

	mapping_len = size;

	return ptr;
}

/* io_mremap : remaps a virtual memory address of a size to a new size */
void *io_mremap(void *oldaddr, size_t oldsize, size_t newsize)
{
	void *ret;

#if WIN32
	// for win32, we use the trick that we store the file name
	// :)
	ret = MAP_FAILED;
#else
	ret = mremap(oldaddr, oldsize, newsize, MMAP_FLAGS);
#endif

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
#ifdef WIN32
	ret = FlushViewOfFile(ptr, len);
#else
	ret = msync(((char *)ptr) - addlsize, len + addlsize, flags);
#endif

	if (ret) {
		io_errnohandle();
	}

	return ret;
}

/* io_getpagesize : wrapper for os independence */
static int io_getpagesize()
{
#if WIN32
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwPageSize;
#else
	return getpagesize();
#endif
}

#ifdef WIN32
/* munmap : windows wrapper for munmap */
int munmap(void *start, size_t length)
{
	return !UnmapViewOfFile(start);
}

#endif

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

/* io_open : wrapper for "our" open call */
int io_open(char *name)
{
	int rc;

#if WIN32
	rc = open(name, _O_RDWR | _O_CREAT, _S_IREAD | _S_IWRITE);
#else
	rc = open(name, O_RDWR | O_CREAT, 0666);
#endif

	if (rc < 0) {
		io_errnohandle();
	}

	strncpy(io_filename, name, sizeof(io_filename));

	return rc;
}

/* io_getsize : returns the size of the file */
u64 io_getsize()
{
	struct stat st;
	stat(io_getfilename(), &st);
	return st.st_size;
}

/* io_getfilename : returns pointer to the file name */
char *io_getfilename()
{
	return &io_filename[0];
}

int io_resize(int fd, size_t size)
{
	int rc;

#if WIN32
	LARGE_INTEGER theint;
	theint.QuadPart = size;
	rc = SetFilePointerEx((HANDLE)_get_osfhandle(fd), theint, NULL, FILE_BEGIN);
	rc = SetEndOfFile((HANDLE)_get_osfhandle(fd));
	if (!rc) {
		io_errnohandle(); // TODO might need different error checking for this
	}
#else
	rc = posix_fallocate(fd, 0, size);
	if (rc < 0) {
		io_errnohandle();
	}
#endif

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

/* io_exists : returns true if a file exists */
int io_access(char *path)
{
#if WIN32
	return _access(path, 0) == 0;
#else
	return access(path, F_OK) == 0;
#endif
}

/* io_readfile : reads file 'path' into memory and NULL terminates */
char *io_readfile(char *path)
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


/* lump functions */

/* io_lumpcheck : 0 if file has correct magic, else if it doesn't */
int io_lumpcheck(void *ptr)
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

/* io_lumpgetbase : retrieves the base pointer for a lump */
void *io_lumpgetbase(void *base, int lumpid)
{
	struct lump_header_t *hdr;

	hdr = base;

	return (void *)(((char *)base) + hdr->lump[lumpid].offset);
}

/* io_lumpgetidx : gets a pointer to an array in the lump */
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

/* io_lumpgetmeta : returns a pointer to a lumpmeta_t for a given lumpid */
struct lumpmeta_t *io_lumpgetmeta(void *base, int lumpid)
{
	struct lump_header_t *hdr;

	hdr = base;

	return (struct lumpmeta_t *)(((char *)base) + hdr->lump[lumpid].offset);
}


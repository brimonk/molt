/* input output header */

#ifndef MOLT_IO
#define MOLT_IO

#include "common.h"

/* memory mapping wrappers */
void *io_mmap(int fd, size_t size);
void *io_mremap(void *oldaddr, size_t oldsize, size_t newsize);
int io_mssync(void *base, void *ptr, size_t len);
int io_masync(void *base, void *ptr, size_t len);
int io_munmap(void *ptr);

/* file wrapped operations */
int io_open(char *name);
int io_resize(int fd, size_t size);
int io_close(int fd);
u64 io_getsize();
char *io_getfilename();

/* lump helper funcs */

/* io_lumpcheck : 0 if file has correct magic, else if it doesn't */
int io_lumpcheck(void *ptr);
/* io_lumprecnum : returns the number of records that a given lump has */
int io_lumprecnum(void *base, int lumpid);
/* io_lumpgetbase : retrieves the base pointer for a lump */
void *io_lumpgetbase(void *base, int lumpid);
/* io_lumpgetidx : gets a pointer to an array in the lump */
void *io_lumpgetidx(void *base, int lumpid, int idx);

/* console message wrappers */
int io_fprintf(FILE *fp, const char *fmt, ...);

#endif

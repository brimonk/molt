/* input output header */

#ifndef MOLT_IO
#define MOLT_IO

/* memory mapping wrappers */
void *io_mmap(int fd, size_t size);
int io_munmap(void *ptr);
int io_mssync(void *base, void *ptr, size_t len);
int io_masync(void *base, void *ptr, size_t len);

/* file wrapped operations */
int io_open(char *name);
int io_resize(int fd, size_t size);
int io_close(int fd);
char *io_getfilename();

/* console message wrappers */
int io_fprintf(FILE *fp, const char *fmt, ...);

/* lump helper funcs */
int io_lumpcheck(void *ptr);
int io_lumpsetup(void *ptr);
void *io_lumpgetid(void *ptr, int idx);
int io_lumprecnum(void *ptr, int idx);

#endif

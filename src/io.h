/* input output header */

#ifndef MOLT_IO
#define MOLT_IO

/* memory mapping wrappers */
void *io_mmap(int fd, size_t size);
int io_munmap(void *ptr);

/* file wrapped operations */
int io_open(char *name);
int io_resize(int fd, size_t size);
int io_close(int fd);

/* lump helper funcs */
int io_lumpcheck(void *ptr);
int io_lumpsetup(void *ptr);
void *io_lumpgetid(void *ptr, int idx);
int io_lumprecnum(void *ptr, int idx);

#endif

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

#endif

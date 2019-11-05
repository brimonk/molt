#ifndef SYS_H
#define SYS_H

#include "common.h"

/* sys_mmap : system wrapper for mmap */
void *sys_mmap(int fd, size_t size);

/* sys_mmsync : system wrapper for mssync (sync memory mapped page) */
int sys_mssync(void *base, void *ptr, size_t len);

/* sys_mmsync : system wrapper for mssync (async sync memory mapped page) */
int sys_masync(void *base, void *ptr, size_t len);

/* sys_msync : msync wrapper - ensures we write with a PAGESIZE multiple */
int sys_msync(void *base, void *ptr, size_t len, int flags);

/* sys_munmap : system wrapper for munmap */
int sys_munmap(void *ptr);

/* sys_getpagesize : system wrapper for getpagesize */
int sys_getpagesize();

/* sys_open : system wrapper for open */
int sys_open(char *name);

/* sys_open : system wrapper for close */
int sys_close(int fd);

/* sys_resize : system wrapper for resizing a file */
int sys_resize(int fd, size_t size);

/* sys_resize : system wrapper for getting the size of a file */
u64 sys_getsize();

/* sys_exists : system wrapper to see if a file currently exists */
int sys_exists(char *path);

/* sys_readfile : reads an entire file into a memory buffer */
char *sys_readfile(char *path);

/* sys_lumpcheck : 0 if file has correct magic, else if it doesn't */
int sys_lumpcheck(void *ptr);

/* sys_lumprecnum : returns the number of records a given lump has */
int sys_lumprecnum(void *base, int lumpid);

/* sys_lumpgetbase : retrieves the base pointer for a lump */
void *sys_lumpgetbase(void *base, int lumpid);

/* sys_lumpgetidx : gets a pointer to an array in the lump */
void *sys_lumpgetidx(void *base, int lumpid, int idx);

/* io_lumpgetmeta : returns a pointer to a lumpmeta_t for a given lumpid */
struct lumpmeta_t *sys_lumpgetmeta(void *base, int lumpid);

/* sys_libopen : sys wrapper for dlopen */
void *sys_libopen(char *filename);

/* sys_libsym : sys wrapper for dlsym */
void *sys_libsym(void *handle, char *symbol);

/* sys_libclose : sys wrapper for dlclose */
int sys_libclose(void *handle);

#endif // SYS_H


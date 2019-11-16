#ifndef SYS_H
#define SYS_H

/*
 * Brian Chrzanowski
 * Fri Nov 15, 2019 15:08
 *
 * Generic System Interface
 */

#include <stdio.h>

#include "common.h"

struct sys_file {
	int fd;
	char name[256];
};
typedef struct sys_file sys_file;

/* sys_getpagesize : system wrapper for getpagesize */
int sys_getpagesize();

/* sys_open : system wrapper for open */
sys_file sys_open(char *name);

/* sys_open : system wrapper for close */
int sys_close(sys_file fd);

/* sys_getsize : system wrapper for getting the size of a file */
u64 sys_getsize(sys_file fd);

/* sys_read : wrapper for fread */
size_t sys_read(sys_file fd, size_t start, size_t len, void *ptr);

/* sys_write : wrapper for fwrite */
size_t sys_write(sys_file fd, size_t start, size_t len, void *ptr);

/* sys_readfile : reads an entire file into a memory buffer */
char *sys_readfile(char *path);

/* sys_libopen : sys wrapper for dlopen */
void *sys_libopen(char *filename);

/* sys_libsym : sys wrapper for dlsym */
void *sys_libsym(void *handle, char *symbol);

/* sys_libclose : sys wrapper for dlclose */
int sys_libclose(void *handle);

/* sys_timestamp : gets the current timestamp */
int sys_timestamp(u64 *sec, u64 *usec);

int sys_bipopen(FILE **readfp, FILE **writefp, char *command);

#endif // SYS_H


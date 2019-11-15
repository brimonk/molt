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

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <dlfcn.h>

#include "common.h"
#include "sys.h"

static void sys_errorhandle()
{
	fprintf(stderr, "%s", strerror(errno));
}

/* sys_getpagesize : system wrapper for getpagesize */
int sys_getpagesize()
{
	return getpagesize();
}

/* sys_open : system wrapper for open */
sys_file sys_open(char *name)
{
	sys_file fd;

	fd.fd = open(name, O_RDWR|O_CREAT, 0666);

	if (fd.fd < 0) {
		sys_errorhandle();
	} else {
		strncpy(fd.name, name, sizeof(fd.name));
	}

	return fd;
}

/* sys_open : system wrapper for close */
int sys_close(sys_file fd)
{
	int rc;

	rc = close(fd.fd);

	if (rc < 0) {
		sys_errorhandle();
	}

	return rc;
}

/* sys_read : wrapper for fread */
size_t sys_read(sys_file fd, size_t start, size_t len, void *ptr)
{
	off_t off;
	size_t bytes;
	int rc;

	off = lseek(fd.fd, start, SEEK_SET);
	if (off == (off_t)-1) {
		sys_errorhandle();
		return -1;
	}

	bytes = read(fd.fd, ptr, len);
	if (bytes != len) {
		sys_errorhandle();
		return -1;
	}

	rc = fsync(fd.fd);

	if (rc < 0) {
		sys_errorhandle();
	}

	return bytes;
}

/* sys_write : wrapper for fwrite */
size_t sys_write(sys_file fd, size_t start, size_t len, void *ptr)
{
	off_t off;
	size_t bytes;
	int rc;

	off = lseek(fd.fd, start, SEEK_SET);
	if (off == (off_t)-1) {
		sys_errorhandle();
		return -1;
	}

	bytes = write(fd.fd, ptr, len);
	if (bytes != len) {
		sys_errorhandle();
		return -1;
	}

	rc = fsync(fd.fd);
	if (rc < 0) {
		sys_errorhandle();
	}

	return bytes;
}

/* sys_getsize : system wrapper for getting the size of a file */
u64 sys_getsize(sys_file fd)
{
	struct stat st;
	stat(fd.name, &st);
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

/* sys_timestamp : gets the current timestamp */
int sys_timestamp(u64 *sec, u64 *usec)
{
	struct timeval tv;
	struct timezone tz;
	int rc;

	rc = gettimeofday(&tv, &tz);

	if (sec) {
		*sec = tv.tv_sec;
	}

	if (usec) {
		*usec = tv.tv_usec;
	}

	return 0;
}


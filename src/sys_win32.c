/*
 * Brian Chrzanowski
 * Tue Nov 19, 2019 15:02
 *
 * Win32 Platform Layer
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <errno.h>

#if defined(_MSC_VER) || defined(_MSC_EXTENSION)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
  #define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif

#include "common.h"

#include "sys.h"

struct sys_file {
	int fd;
	char name[BUFSMALL];
};

struct sys_thread {
	HANDLE thread;
	void *(*func)(void *arg);
	void *arg;
};

/* sys_errorhandle : mirrors the linux errno handler */
static void sys_errorhandle()
{
	fprintf(stderr, "%s\n", strerror(errno));
}

/* sys_lasterror : handles errors that aren't propogated through win32 errno */
static void sys_lasterror()
{
	// TODO (brian) handle the
}

/* sys_open : system wrapper for open */
sys_file *sys_open(char *name)
{
	struct sys_file *fd;

	fd = calloc(1, sizeof(struct sys_file));

	fd->fd = _open(name, O_RDWR|O_CREAT|O_TRUNC|O_BINARY, _S_IREAD|_S_IWRITE);

	if (fd->fd < 0) {
		sys_errorhandle();
	} else {
		strncpy(fd->name, name, sizeof(fd->name));
	}

	return fd;
}

/* sys_open : system wrapper for close */
int sys_close(sys_file *fd)
{
	int rc;

	rc = _close(fd->fd);

	if (rc < 0) {
		sys_errorhandle();
	}

	return rc;
}

/* sys_getsize : system wrapper for getting the size of a file */
u64 sys_getsize(sys_file *fd)
{
	struct stat st;
	stat(fd->name, &st);
	return st.st_size;
}

/* sys_read : wrapper for fread */
size_t sys_read(sys_file *fd, size_t start, size_t len, void *ptr)
{
	__int64 off;
	size_t bytes;
	int rc;

	off = _lseek(fd->fd, start, SEEK_SET);
	if (off == -1L) {
		sys_errorhandle();
		return -1;
	}

	bytes = _read(fd->fd, ptr, len);
	if (bytes != len) {
		sys_errorhandle();
		return -1;
	}

	rc = _commit(fd->fd);
	if (rc < 0) {
		sys_errorhandle();
	}

	return bytes;
}

/* sys_write : wrapper for fwrite */
size_t sys_write(sys_file *fd, size_t start, size_t len, void *ptr)
{
	__int64 off;
	size_t bytes;
	int rc;

	off = _lseek(fd->fd, start, SEEK_SET);
	if (off == -1L) {
		sys_errorhandle();
		return -1;
	}

	bytes = _write(fd->fd, ptr, len);
	if (bytes != len) {
		sys_errorhandle();
		return -1;
	}

	rc = _commit(fd->fd);
	if (rc < 0) {
		sys_errorhandle();
	}

	return bytes;
}

/* sys_exists : system wrapper to see if a file currently exists */
int sys_exists(char *path)
{
	return _access(path, 0);
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

	p = LoadLibraryA(filename);
	if (p == NULL) {
		sys_lasterror();
	}

	return p;
}

/* sys_libsym : sys wrapper for dlsym */
void *sys_libsym(void *handle, char *symbol)
{
	void *p;

	p = GetProcAddress(handle, symbol);
	if (p == NULL) {
		sys_lasterror();
	}

	return p;
}

/* sys_libclose : sys wrapper for dlclose */
int sys_libclose(void *handle)
{
	// we attempt to return -1 from these functions if they fail
	return FreeLibrary(handle) == 0 ? -1 : 0;
}

/* sys_timestamp : gets the current timestamp */
int sys_timestamp(u64 *sec, u64 *usec)
{
	FILETIME ft;
	unsigned __int64 tmpres;

	GetSystemTimeAsFileTime(&ft);

	tmpres = 0;
	tmpres |= ft.dwHighDateTime;
	tmpres <<= 32;
	tmpres |= ft.dwLowDateTime;

	// convert filetime to unix epoch
	tmpres -= DELTA_EPOCH_IN_MICROSECS;
	tmpres /= 10;
	if (sec) {
		*sec = (long)(tmpres / 1000000UL);
	}

	if (usec) {
		*usec = (long)(tmpres % 1000000UL);
	}

	return 0;
}

/* sys_threadwrap : wrapper to get pairity with the pthread functionality */
static DWORD sys_threadwrap(void *arg)
{
	struct sys_thread *thread;
	void *p;

	thread = arg;

	p = thread->func(thread->arg);

	ExitThread(0);
}

/* sys_threadcreate : creates a new thread */
struct sys_thread *sys_threadcreate()
{
	return calloc(1, sizeof(struct sys_thread));
}

/* sys_threadfree : frees the thread */
int sys_threadfree(struct sys_thread *thread)
{
	free(thread);
	return 0;
}

/* sys_threadsetfunc : sets the function for the thread */
int sys_threadsetfunc(struct sys_thread *thread, void *(*func)(void *arg))
{
	thread->func = func;
	return 0;
}

/* sys_threadsetarg : sets the argument for the thread */
int sys_threadsetarg(struct sys_thread *thread, void *arg)
{
	thread->arg = arg;
	return 0;
}

/* sys_threadstart : actually starts the thread */
int sys_threadstart(struct sys_thread *thread)
{
	thread->thread = CreateThread(NULL, 0, sys_threadwrap, thread, 0, NULL);
	return 0;
}

/* sys_threadwait : waits for the given thread to exit, then returns */
int sys_threadwait(struct sys_thread *thread)
{
	WaitForSingleObject(thread->thread, INFINITE);
	return 0;
}

/* sys_bipopen : creates a "bi directional" popen */
int sys_bipopen(FILE **readfp, FILE **writefp, char *command)
{
	return 0;
}


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
	DWORD error;
	char *errmsg;

	error = GetLastError();

	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&errmsg, 0, NULL);

	fprintf(stderr, "%s\n", errmsg);

	LocalFree(errmsg);
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

#if 1
int sys_timestamp(u64 *sec, u64 *usec)
{
	// NOTE (brian) stolen from stack overflow
	//   https://stackoverflow.com/questions/20370920/convert-current-time-from-windows-to-unix-timestamp-in-c-or-c

   //Get the number of seconds since January 1, 1970 12:00am UTC
   //Code released into public domain; no attribution required.

   const __int64 UNIX_TIME_START = 0x019DB1DED53E8000; //January 1, 1970 (start of Unix epoch) in "ticks"
   const __int64 TICKS_PER_SECOND = 10000000; //a tick is 100ns
   __int64 TheValue;

   FILETIME ft;
   GetSystemTimeAsFileTime(&ft); //returns ticks in UTC

   // Copy the low and high parts of FILETIME into a LARGE_INTEGER
   // This is so we can access the full 64-bits as an Int64 without causing an alignment fault
   LARGE_INTEGER li;
   li.LowPart  = ft.dwLowDateTime;
   li.HighPart = ft.dwHighDateTime;

   // Convert ticks since 1/1/1970 into seconds
   TheValue = (li.QuadPart - UNIX_TIME_START) / TICKS_PER_SECOND;

   if (sec) {
	   *sec = TheValue;
   }

   return 0;
}
#else
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
#endif

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

/* sys_numcores : returns the number of cores available in the system */
int sys_numcores(void)
{
	SYSTEM_INFO info;

	GetSystemInfo(&info);

	return info.dwNumberOfProcessors;
}

/* sys_bipopen : creates a "bi directional" popen */
int sys_bipopen(FILE **readfp, FILE **writefp, char *command)
{
	SECURITY_ATTRIBUTES attribs;
	HANDLE cpipe_in;
	HANDLE cpipe_out;
	HANDLE ppipe_in;
	HANDLE ppipe_out;
	PROCESS_INFORMATION procinfo;
	STARTUPINFO startupinfo;
	int rc;
	int ppipe_fdin;
	int ppipe_fdout;

	// windows is weird

	// set the bInheritHandle so pipe handles are inherited
	attribs.nLength = sizeof(SECURITY_ATTRIBUTES);
	attribs.bInheritHandle = TRUE;
	attribs.lpSecurityDescriptor = NULL;

	// create a pipe for the child's stdout
	rc = CreatePipe(&cpipe_in, &ppipe_out, &attribs, 0);
	if (!rc) {
		sys_lasterror();
		return -1;
	}

	// and make sure the child won't inherit the pipe for STDOUT
	rc = SetHandleInformation(ppipe_out, HANDLE_FLAG_INHERIT, 0);
	if (!rc) {
		sys_lasterror();
		return -1;
	}

	// create a pipe for the child process's STDIN
	rc = CreatePipe(&ppipe_in, &cpipe_out, &attribs, 0);
	if (!rc) {
		sys_lasterror();
		return -1;
	}

	// and ensure the write handle for parent's input isn't inherited
	rc = SetHandleInformation(ppipe_in, HANDLE_FLAG_INHERIT, 0);
	if (!rc) {
		sys_lasterror();
		return -1;
	}

	memset(&procinfo, 0, sizeof(procinfo));
	memset(&startupinfo, 0, sizeof(startupinfo));

	startupinfo.cb = sizeof(STARTUPINFO);
	startupinfo.hStdError = cpipe_out;
	startupinfo.hStdOutput = cpipe_out;
	startupinfo.hStdInput = cpipe_in;
	startupinfo.dwFlags |= STARTF_USESTDHANDLES;

	rc = CreateProcess(NULL, command, NULL, NULL, TRUE, 0, NULL, NULL, &startupinfo, &procinfo);
	if (!rc) {
		sys_lasterror();
		return -1;
	}

	// close handle to the child's process and thread
	CloseHandle(procinfo.hProcess);
	CloseHandle(procinfo.hThread);

	// now that we've finally done the equivalent of calling fork(),
	// we have to setup our C-friendly FILE *'s.
	ppipe_fdin  = _open_osfhandle((intptr_t)ppipe_in, _O_RDONLY);
	ppipe_fdout = _open_osfhandle((intptr_t)ppipe_out, _O_WRONLY);

	*readfp = _fdopen(ppipe_fdin, "rt");
	*writefp = _fdopen(ppipe_fdout, "wt");

	return 0;
}


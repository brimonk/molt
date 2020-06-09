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

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <dlfcn.h>
#include <pthread.h>

#include "common.h"
#include "sys.h"

struct sys_file {
	int fd;
	char name[BUFSMALL];
};

struct sys_thread {
	pthread_t thread;
	void *(*func)(void *arg);
	void *arg;
};

static void sys_errorhandle()
{
	fprintf(stderr, "%s\n", strerror(errno));
}

/* sys_open : system wrapper for open */
sys_file *sys_open(char *name)
{
	sys_file *fd;

	fd = calloc(1, sizeof(struct sys_file));

	fd->fd = open(name, O_RDWR|O_CREAT|O_TRUNC, 0666);

	if (fd->fd < 0) {
		sys_errorhandle();
		free(fd);
		fd = NULL;
	} else {
		strncpy(fd->name, name, sizeof(fd->name));
	}

	return fd;
}

/* sys_open : system wrapper for close */
int sys_close(sys_file *fd)
{
	int rc;

	rc = close(fd->fd);

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
	off_t off;
	size_t bytes;
	int rc;

	off = lseek(fd->fd, start, SEEK_SET);
	if (off == (off_t)-1) {
		sys_errorhandle();
		return -1;
	}

	bytes = read(fd->fd, ptr, len);
	if (bytes != len) {
		sys_errorhandle();
		return -1;
	}

	rc = fsync(fd->fd);

	if (rc < 0) {
		sys_errorhandle();
	}

	return bytes;
}

/* sys_write : wrapper for fwrite */
size_t sys_write(sys_file *fd, size_t start, size_t len, void *ptr)
{
	off_t off;
	size_t bytes;
	int rc;

	off = lseek(fd->fd, start, SEEK_SET);
	if (off == (off_t)-1) {
		sys_errorhandle();
		return -1;
	}

	bytes = write(fd->fd, ptr, len);
	if (bytes != len) {
		sys_errorhandle();
		return -1;
	}

	rc = fsync(fd->fd);
	if (rc < 0) {
		sys_errorhandle();
	}

	return bytes;
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
	if (rc < 0) {
		sys_errorhandle();
	}

	if (sec) {
		*sec = tv.tv_sec;
	}

	if (usec) {
		*usec = tv.tv_usec;
	}

	return 0;
}

/* sys_threadwrap : wrapper to get pairity with the pthread functionality */
static void *sys_threadwrap(void *arg)
{
	struct sys_thread *thread;
	void *p;

	thread = arg;

	p = thread->func(thread->arg);

	return p;
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
	int rc;

	rc = pthread_create(&thread->thread, NULL, sys_threadwrap, thread);
	if (rc != 0) {
		sys_errorhandle();
	}

	return rc != 0;
}

/* sys_threadwait : waits for the given thread to exit, then cleans up */
int sys_threadwait(struct sys_thread *thread)
{
	void *ret;
	int rc;

	rc = pthread_join(thread->thread, &ret);
	if (rc != 0) {
		sys_errorhandle();
	}

	return rc != 0;
}

/* sigchld_handler : handles 'SIGCHLD' signal to prevent zombies from bipopen */
static void sigchld_handler(int signum)
{
	if (signum == SIGCHLD) {
		wait(NULL);
	}
}

/* sys_bipopen : system's bi-directional popen */
int sys_bipopen(FILE **readfp, FILE **writefp, char *command)
{
	// NOTE (brian)
	// on systems that have a bidirectional popen, this function
	// could probably be removed

	pid_t pid;
	int pipes[4];
	char *args[BUFSMALL];
	char buf[BUFLARGE];
	char *s;
	int i, rc;

	strncpy(buf, command, sizeof(buf));

	memset(args, 0, sizeof(args));

	// parse our arguments for exec
	s = strtok(buf, " ");
	for (i = 0; s && i < ARRSIZE(args); i++) {
		args[i] = s;
		s = strtok(NULL, " ");
	}

	rc = pipe(&pipes[0]); // parent read, child write pipes
	if (rc < 0) {
		sys_errorhandle();
	}
	rc = pipe(&pipes[2]); // child read, parent write pipes
	if (rc < 0) {
		sys_errorhandle();
	}

	if (0 < (pid = fork())) { // parent
		// install signal handler to clean up the child
		signal(SIGCHLD, sigchld_handler);

		*readfp  = fdopen(pipes[0], "r");
		*writefp = fdopen(pipes[3], "w");

		rc = setvbuf(*readfp, NULL, _IONBF, 0);
		rc = setvbuf(*writefp, NULL, _IONBF, 0);

		rc = close(pipes[1]);
		if (rc < 0) {
			sys_errorhandle();
		}
		rc = close(pipes[2]);
		if (rc < 0) {
			sys_errorhandle();
		}

		return 0;
	} else if (pid == 0) { // child

		// close parent pipes, we don't need them
		close(pipes[0]);
		close(pipes[3]);

		dup2(pipes[1], STDOUT_FILENO);
		dup2(pipes[2], STDIN_FILENO);

		close(pipes[1]);
		close(pipes[2]);

		rc = execvp(args[0], args);

		printf("child fork() failed : %s\n", strerror(errno));

		exit(0); // kill the child
	} else {
		sys_errorhandle();
		return -1;
	}

	return 0;
}


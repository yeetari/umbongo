#pragma once

#include <limits.h> // IWYU pragma: keep
#include <stddef.h> // IWYU pragma: keep
#include <sys/cdefs.h>
#include <sys/types.h> // IWYU pragma: keep

__BEGIN_DECLS

extern char **environ;

int access(const char *, int);
int close(int);
void _exit(int);
pid_t getpid(void);
int unlink(const char *);

int dup(int);
int dup2(int, int);

int isatty(int);
int pipe(int[2]);
unsigned sleep(unsigned);

pid_t fork(void);

int execv(const char *, char *const[]);
int execve(const char *, char *const[], char *const[]);
int execvp(const char *, char *const[]);

int chdir(const char *);
int rmdir(const char *);

off_t lseek(int, off_t, int);
ssize_t read(int, void *, size_t);
ssize_t write(int, const void *, size_t);

__END_DECLS

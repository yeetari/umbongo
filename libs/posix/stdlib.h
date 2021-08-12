#pragma once

#include <stddef.h> // IWYU pragma: keep
#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

__attribute__((noreturn)) void abort(void);
__attribute__((noreturn)) void exit(int);

__attribute__((malloc)) __attribute__((alloc_size(1))) void *malloc(size_t);
__attribute__((malloc)) __attribute__((alloc_size(1, 2))) void *calloc(size_t, size_t);
__attribute__((alloc_size(2))) void *realloc(void *, size_t);
void free(void *);

int atoi(const char *);
long atol(const char *);
double atof(const char *);
unsigned long strtoul(const char *, char **, int);
unsigned long long strtoull(const char *, char **, int);

char *getcwd(char *, size_t);
char *getenv(const char *);
size_t mbstowcs(uint16_t *, const char *, size_t);
char *mktemp(char *);
void qsort(void *, size_t, size_t, int (*)(const void *, const void *));

__END_DECLS

#pragma once

#include <limits.h> // IWYU pragma: keep
#include <stdarg.h> // IWYU pragma: keep
#include <stddef.h> // IWYU pragma: keep
#include <sys/cdefs.h>

__BEGIN_DECLS

#define BUFSIZ 1024
#define EOF (-1)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct FILE FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

FILE *fopen(const char *, const char *);
int feof(FILE *);
int ferror(FILE *);
int fflush(FILE *);
int fclose(FILE *);

int fseek(FILE *, long, int);
long ftell(FILE *);
void rewind(FILE *);

char *fgets(char *, int, FILE *);
size_t fread(void *, size_t, size_t, FILE *);
size_t fwrite(const void *, size_t, size_t, FILE *);

int fgetc(FILE *);
int fputc(int, FILE *);
int fputs(const char *, FILE *);
int getc(FILE *);
int putc(int, FILE *);
int puts(const char *);
int ungetc(int, FILE *);

void clearerr(FILE *);
void perror(const char *);
int fileno(FILE *);
int getchar(void);
int putchar(int);
FILE *fdopen(int, const char *);

__attribute__((format(printf, 1, 2))) int printf(const char *, ...);
__attribute__((format(printf, 2, 3))) int fprintf(FILE *, const char *, ...);
__attribute__((format(printf, 2, 3))) int sprintf(char *, const char *, ...);
__attribute__((format(printf, 3, 4))) int snprintf(char *, size_t, const char *, ...);

__attribute__((format(printf, 1, 0))) int vprintf(const char *, va_list);
__attribute__((format(printf, 2, 0))) int vfprintf(FILE *, const char *, va_list);
__attribute__((format(printf, 2, 0))) int vsprintf(char *, const char *, va_list);
__attribute__((format(printf, 3, 0))) int vsnprintf(char *, size_t, const char *, va_list);

__attribute__((format(scanf, 1, 2))) int scanf(const char *, ...);
__attribute__((format(scanf, 2, 3))) int fscanf(FILE *, const char *, ...);
__attribute__((format(scanf, 2, 3))) int sscanf(const char *, const char *, ...);

__attribute__((format(scanf, 1, 0))) int vscanf(const char *, va_list);
__attribute__((format(scanf, 2, 0))) int vfscanf(FILE *, const char *, va_list);
__attribute__((format(scanf, 2, 0))) int vsscanf(const char *, const char *, va_list);

int remove(const char *);
FILE *tmpfile(void);

__END_DECLS

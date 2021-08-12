#pragma once

#include <stddef.h> // IWYU pragma: keep
#include <sys/cdefs.h>

__BEGIN_DECLS

#define LC_ALL 0
#define LC_COLLATE 1
#define LC_CTYPE 2
#define LC_MESSAGES 3
#define LC_MONETARY 4
#define LC_NUMERIC 5
#define LC_TIME 6

char *setlocale(int, const char *);

__END_DECLS

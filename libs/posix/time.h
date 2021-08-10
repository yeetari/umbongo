#pragma once

#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

typedef int64_t time_t;

time_t time(time_t *);
char *ctime(const time_t *);

__END_DECLS

#pragma once

#ifdef __x86_64__
#define ARCH_IS_amd64() 1
#else
#define ARCH_IS_amd64() 0
#endif

#define ARCH(arch) (ARCH_IS_##arch())

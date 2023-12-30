#pragma once

#include <kernel/arch/arch.hh>

// IWYU pragma: begin_keep
#if ARCH(amd64)
#include <kernel/arch/amd64/register_state.hh>
#else
#error Unhandled architecture
#endif
// IWYU pragma: end_keep

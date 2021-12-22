add_library(mmio INTERFACE)
target_link_libraries(mmio INTERFACE core)
target_sources(mmio INTERFACE Mmio.hh)

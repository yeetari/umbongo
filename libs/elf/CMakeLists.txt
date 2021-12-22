add_library(elf INTERFACE)
target_link_libraries(elf INTERFACE ustd)
target_sources(elf INTERFACE Elf.hh)

add_library(ustd Memory.cc)
target_compile_features(ustd PRIVATE cxx_std_20)
target_compile_options(ustd PRIVATE
    -ffreestanding -fno-exceptions -fno-rtti
    -nostdinc++ -nostdlib)
target_gc_sections(ustd)
target_link_options(ustd PRIVATE LINKER:-nostdlib)

add_library(ustd Abi.cc Assert.cc Memory.cc String.cc)
target_compile_definitions(ustd PUBLIC
    $<$<BOOL:${ENABLE_ASSERTIONS}>:ASSERTIONS>
    $<$<BOOL:${ENABLE_ASSERTIONS_PEDANTIC}>:ASSERTIONS_PEDANTIC>)
target_gc_sections(ustd)

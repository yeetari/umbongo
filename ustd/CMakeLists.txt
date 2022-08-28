set(SOURCES
    Builtin.cc
    String.cc
    StringBuilder.cc
    StringView.cc)

add_library(ustd ${SOURCES})
install(TARGETS ustd DESTINATION lib)
target_compile_definitions(ustd PUBLIC
    $<$<BOOL:${ENABLE_ASSERTIONS}>:ASSERTIONS>
    $<$<BOOL:${ENABLE_ASSERTIONS_PEDANTIC}>:ASSERTIONS_PEDANTIC>)
target_gc_sections(ustd)

add_library(ustd-no-sse ${SOURCES})
target_compile_definitions(ustd-no-sse PUBLIC
    $<$<BOOL:${ENABLE_ASSERTIONS}>:ASSERTIONS>
    $<$<BOOL:${ENABLE_ASSERTIONS_PEDANTIC}>:ASSERTIONS_PEDANTIC>)
target_compile_options(ustd-no-sse PRIVATE -mno-sse)
target_gc_sections(ustd-no-sse)

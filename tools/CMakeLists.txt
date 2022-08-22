project(tools)

add_executable(mkfont mkfont.cc)
target_compile_options(mkfont PRIVATE -Wno-everything)
target_include_directories(mkfont PRIVATE /usr/include/freetype2)
target_link_libraries(mkfont PRIVATE freetype)

add_executable(mkfs mkfs.cc)
target_compile_options(mkfs PRIVATE -Wno-everything)
target_include_directories(mkfs PRIVATE ..)

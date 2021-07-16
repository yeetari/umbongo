project(tools)

add_executable(font-generator FontGenerator.cc)
target_compile_options(font-generator PRIVATE -Wno-everything)
target_include_directories(font-generator PRIVATE /usr/include/freetype2)
target_link_libraries(font-generator PRIVATE freetype)

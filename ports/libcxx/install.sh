cmake ../libcxx -GNinja -DCMAKE_SYSROOT="~/projects/umbongo/build-release/sysroot"
 -DCMAKE_TOOLCHAIN_FILE="~/projects/umbongo/toolchain/clang.cmake"
 -DLIBCXX_ENABLE_SHARED=OFF
 -DLIBCXX_ENABLE_STATIC=ON
 -DLIBCXX_ENABLE_FILESYSTEM=OFF
 -DLIBCXX_ENABLE_INCOMPLETE_FEATURES=OFF
 -DLIBCXX_ENABLE_EXPERIMENTAL_LIBRARY=OFF
 -DLIBCXX_ENABLE_EXCEPTIONS=OFF
 -DLIBCXX_INCLUDE_TESTS=OFF
 -DLIBCXX_INCLUDE_BENCHMARKS=OFF
 -DCMAKE_C_FLAGS="-nostdlib -I~/projects/umbongo/libs/posix"
 -DCMAKE_CXX_FLAGS="-nostdlib -I~/projects/umbongo/libs/posix -ferror-limit=99999"
ninja -k 0 2>&1 >errors

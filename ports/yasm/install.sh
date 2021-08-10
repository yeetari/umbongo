set -e

if [ "$#" -ne 1 ]; then
    printf "Usage: $0 <build-dir>\n"
    exit 1
fi

SYSROOT=$(realpath $1/sysroot)
VERSION=1.3.0
cd $(dirname $0)

DIRECTORY=yasm-$VERSION
TARBALL=yasm-$VERSION.tar.gz

if [ ! -d $DIRECTORY ]; then
    if [ ! -f $TARBALL ]; then
        curl -LSo "$TARBALL" https://github.com/yasm/yasm/releases/download/v$VERSION/$TARBALL
    fi
    tar xvf $TARBALL
fi
cd $DIRECTORY
for path in ../patches/*.patch; do
    patch -p1 -N < "$path" || true
done

export AR=llvm-ar
export CC="clang --sysroot=$SYSROOT -fpic -mno-sse"
export CPPFLAGS="-I$SYSROOT/../../libs/posix"
export LDFLAGS="-fuse-ld=lld -nostdlib -pie -lcore -lposix -lustd -Xlinker -dynamic-linker -Xlinker /dynamic-linker"
export RANLIB=llvm-ranlib
export STRIP=llvm-strip
./configure --host=x86_64-umbongo --prefix=$SYSROOT
make clean && make -j$(nproc)
cp yasm $SYSROOT

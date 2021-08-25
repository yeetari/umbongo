set -e

if [ "$#" -ne 1 ]; then
    printf "Usage: $0 <build-dir>\n"
    exit 1
fi

SYSROOT=$(realpath $1/sysroot)
VERSION=2.37
cd $(dirname $0)

DIRECTORY=binutils-$VERSION
TARBALL=binutils-$VERSION.tar.xz

if [ ! -d $DIRECTORY ]; then
    if [ ! -f $TARBALL ]; then
        curl -LSo "$TARBALL" https://ftpmirror.gnu.org/gnu/binutils/$TARBALL
    fi
    tar xvf $TARBALL
fi
cd $DIRECTORY
for path in ../patches/*.patch; do
    patch -p1 -N < "$path" || true
done

export AR=llvm-ar
export CC="clang --sysroot=$SYSROOT -fpic -nostdlib"
export LDFLAGS="-Xlinker -dynamic-linker -Xlinker /bin/dynamic-linker -fuse-ld=lld -pie"
export LIBS="-lcore -lposix -lustd"
export RANLIB=llvm-ranlib
export STRIP=llvm-strip
./configure --host=x86_64-pc-umbongo --target=x86_64-pc-umbongo --prefix=$SYSROOT --disable-gdb --disable-nls --disable-werror
make clean && make -j$(nproc)
cp ld/ld-new $SYSROOT/bin/ld

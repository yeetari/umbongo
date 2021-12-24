set -e

if [ "$#" -ne 1 ]; then
    printf "Usage: $0 <build-dir>\n"
    exit 1
fi

SYSROOT=$(realpath $1/sysroot)
cd $(dirname $0)

DIRECTORY=tinycc

if [ ! -d $DIRECTORY ]; then
    git clone https://github.com/TinyCC/tinycc.git
fi
cd $DIRECTORY
git reset --hard dda95e9b0b30771369efe66b4a47e94cf0ca7dc0
for path in ../patches/*.patch; do
    patch -p1 -N < "$path" || true
done

./configure \
    --ar=llvm-ar \
    --cc=clang \
    --extra-cflags="--sysroot=$SYSROOT -fpic -DCONFIG_TCC_STATIC -DCONFIG_TCC_SEMLOCK=0" \
    --extra-ldflags="--sysroot=$SYSROOT -fuse-ld=lld -nostdlib -pie -lcore -lipc -llog -lposix -lustd -Xlinker -dynamic-linker -Xlinker /bin/dynamic-linker" \
    --sysroot=$SYSROOT \
    --elfinterp=/bin/dynamic-linker \
    --sysincludepaths=/include
make clean && make -j$(nproc) tcc
cp tcc $SYSROOT/bin

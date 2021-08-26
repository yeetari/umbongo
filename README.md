# umbongo

An x86 operating system

* 64-bit and UEFI only
* Aiming for a microkernel design, with the kernel's primary focus being IPC
* Work in progress posix layer library that can already host a few ports (ld, tcc, yasm)
* Written in C++ 20

## Building and running

### Prerequisites

Umbongo uses an LLVM toolchain for building. `cmake`, `freetype`, `nasm` (`yasm` should also work), and either `make`
or `ninja` will also be needed. `mtools` and `qemu` are optional and are needed for building a filesystem image and
running in a VM respectively.

#### Gentoo

    emerge \
      app-emulation/qemu \
      dev-lang/nasm \
      dev-util/cmake \
      dev-util/ninja \
      media-libs/freetype \
      sys-devel/clang \
      sys-devel/lld \
      sys-fs/mtools

### Configuring CMake

To configure umbongo, use

    ccmake . -Bbuild -GNinja

which will bring up cmake's terminal interface to allow configuration. Alternatively, options can be passed on the
command line using `-D`:

    cmake . \
      -Bbuild \
      -DBUILD_TYPE=Release \
      -DENABLE_ASSERTIONS=ON \
      -GNinja

### Running with qemu

    cmake --build build --target run

## Installing ports

To install ports, first make sure the sysroot is up-to-date by running

    cmake --build build --target install

(note that the `image` and `run` targets automatically do this)

Then, run the port's `install.sh` script, passing in the build directory. For example, to install `tcc`, you would do

    ./ports/tcc/install.sh build

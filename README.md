# umbongo

An x86 operating system

* 64-bit and UEFI only
* Aiming for a microkernel design, with the kernel's primary focus being IPC
* Work in progress posix layer library that can already host a few ports (ld, tcc, yasm)
* Written in C++ 20

## Building and running

Umbongo uses an LLVM toolchain for building, along with its own build tool for generating `ninja` files. `freetype` and
`nasm` (`yasm` should also work) will also be needed. `mtools` and `qemu` are optional and are needed for building a
filesystem image and running in a VM respectively.

Alternatively, umbongo can be built in a reproducible docker environment ([see below](#building-in-docker)).

### Installing tools and dependencies

#### Gentoo

    emerge -n \
      app-emulation/qemu \
      dev-lang/nasm \
      dev-util/ninja \
      media-libs/freetype \
      sys-devel/clang \
      sys-devel/lld \
      sys-fs/mtools

### Configuring the build

To configure umbongo, call the `configure.py` script with a preset:

    python3 configure.py \
      -B build \
      -p release \
      -e assertions \
      -e kernel_qemu_debug

#### Available options

| Option                   | Description                                      |
|--------------------------|--------------------------------------------------|
| `assertions`             | Enables assertions project-wide                  |
| `assertions_pedantic`    | Enables more aggressive assertions               |
| `kernel_qemu_debug`      | Enables kernel debug logging to port E9          |
| `kernel_stack_protector` | Enables stack smashing protection for the kernel |
| `kernel_ubsan`           | Enables UBSan for the kernel                     |

### Running with qemu

    ninja -C build run

## Installing ports

To install ports, first make sure the sysroot is up-to-date by running

    ninja -C build sysroot

(note that the `image` and `run` targets automatically do this)

Then, run the port's `install.sh` script, passing in the build directory. For example, to install `tcc`, you would do

    ./ports/tcc/install.sh build

## Building in docker

    docker run --rm -u $(id -u) -v $(pwd):/src ghcr.io/yeetari/umbongo:master python3 /src/configure.py \
      -B/src/build \
      -p release \
      -e assertions
    docker run --rm -u $(id -u) -v $(pwd):/src ghcr.io/yeetari/umbongo:master ninja -C /src/build

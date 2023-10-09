# syntax=docker/dockerfile:1

FROM ubuntu:lunar

RUN apt-get update \
 && apt-get install -y \
      ca-certificates \
      gnupg2 \
      software-properties-common \
      wget \
 && wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key \
  | gpg --dearmor - \
  > /usr/share/keyrings/llvm-archive-keyring.gpg \
 && echo 'deb [signed-by=/usr/share/keyrings/llvm-archive-keyring.gpg] https://apt.llvm.org/lunar/ llvm-toolchain-lunar-16 main' > /etc/apt/sources.list.d/llvm.list \
 && apt-get update \
 && apt-get install -y \
      clang-16 \
      clang-format-16 \
      clang-tidy-16 \
      clang-tools-16 \
      cmake \
      git \
      lld-16 \
      libclang-16-dev \
      libfreetype-dev \
      llvm-16-dev \
      nasm \
      ninja-build \
      parallel \
 && rm -rf \
   /var/lib/apt/lists/* \
 && git clone \
   --depth 1 \
   --branch 0.20 \
   https://github.com/include-what-you-use/include-what-you-use.git \
 && cmake \
   -Biwyu-build \
   -GNinja \
   -DCMAKE_BUILD_TYPE=Release \
   -DCMAKE_PREFIX_PATH=/usr/lib/llvm-16 \
   -DCMAKE_INSTALL_PREFIX=/usr \
   ./include-what-you-use \
 && ninja -Ciwyu-build \
 && ninja -Ciwyu-build install \
 && rm /usr/lib/clang/16/include \
 && cp -r iwyu-build/lib/clang/16/include/ /usr/lib/clang/16/include/ \
 && rm -r include-what-you-use iwyu-build \
 && ln -s /usr/bin/llvm-ar-16 /usr/bin/llvm-ar \
 && ln -s /usr/bin/clang-16 /usr/bin/clang \
 && ln -s /usr/bin/clang++-16 /usr/bin/clang++ \
 && ln -s /usr/bin/clang-format-16 /usr/bin/clang-format

#!/bin/bash
set -eu

flags+=("-bios $1")
flags+=("-debugcon stdio")
flags+=("-device qemu-xhci")
flags+=("-device usb-kbd")
flags+=("-device usb-storage,drive=esp")
flags+=("-device virtio-vga,xres=1920,yres=1080")
flags+=("-display sdl")
flags+=("-drive file=fat:rw:$2,format=raw,id=esp,if=none")
flags+=("-machine q35")
flags+=("-nodefaults")
flags+=("-no-reboot")
flags+=("-smp 6")

if [[ -e /dev/kvm ]]; then
    flags+=("-cpu host")
    flags+=("-enable-kvm")
else
    flags+=("-cpu max")
fi

qemu-system-x86_64 ${flags[@]}

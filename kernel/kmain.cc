extern "C" void kmain() {
    const char *msg = "Hello, world!\n\0";
    for (; *msg != '\0'; msg++) {
        asm volatile("out %0, %1" : : "a"(*msg), "Nd"(0xE9));
    }
}

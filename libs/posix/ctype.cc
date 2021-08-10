#include <ctype.h>

#include <sys/cdefs.h>

__BEGIN_DECLS

int isalnum(int c) {
    return isalpha(c) || isdigit(c);
}

int isalpha(int c) {
    return (static_cast<unsigned>(c) | 32u) - 'a' < 26;
}

int isblank(int c) {
    return c == ' ' || c == '\t';
}

int iscntrl(int c) {
    return static_cast<unsigned>(c) < 0x20 || c == 0x7f;
}

int isdigit(int c) {
    return static_cast<unsigned>(c) - '0' < 10;
}

int isgraph(int c) {
    return static_cast<unsigned>(c) - 0x21 < 0x5e;
}

int islower(int c) {
    return static_cast<unsigned>(c) - 'a' < 26;
}

int isprint(int c) {
    return static_cast<unsigned>(c) - 0x20 < 0x5f;
}

int ispuntc(int c) {
    return isgraph(c) && !isalnum(c);
}

int isspace(int c) {
    return c == ' ' || static_cast<unsigned>(c) - '\t' < 5;
}

int isupper(int c) {
    return static_cast<unsigned>(c) - 'A' < 26;
}

int isxdigit(int c) {
    return isdigit(c) || (static_cast<unsigned>(c) | 32u) - 'a' < 6;
}

int tolower(int c) {
    auto ch = static_cast<unsigned>(c);
    return static_cast<int>(isupper(c) ? ch | 32u : ch);
}

int toupper(int c) {
    auto ch = static_cast<unsigned>(c);
    return static_cast<int>(islower(c) ? ch & 0x5fu : ch);
}

__END_DECLS

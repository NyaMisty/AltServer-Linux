#include "windows_shim.h"

std::string StringFromWideString(std::string wideString) {
    return wideString;
}

std::string WideStringFromString(std::string string) {
    return string;
}

char *_itoa(int val, char *buf, int base) {
    if (base != 10) return NULL;
    sprintf(buf, "%d", val);
    return buf;
}
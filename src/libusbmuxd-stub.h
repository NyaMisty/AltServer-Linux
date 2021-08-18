#include <stdint.h>

char pairUDID[256];
char pairDeviceAddress[256];
char pairData[8192];
uint32_t pairDataLen;

#define DEBUG_PRINT(...) do { \
        fprintf(stderr, "[%s:%d] ", __FUNCTION__, __LINE__); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } while (0)

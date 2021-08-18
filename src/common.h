#include <stdint.h>
#include <libimobiledevice/libimobiledevice.h>

char pairUDID[256];
char pairDeviceAddress[256];
char pairData[16384];
uint32_t pairDataLen;

struct UPNPUrls *upnpUrls;
struct IGDdatas *upnpData;
char upnpExternalAddr[40];

#define DEBUG_PRINT(...) do { \
        fprintf(stderr, "[%s:%d] ", __FUNCTION__, __LINE__); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } while (0)

idevice_t g_device;
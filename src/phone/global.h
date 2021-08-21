#include <libimobiledevice/libimobiledevice.h>

#ifdef __cplusplus
extern "C" {
#endif

char pairUDID[256];
char pairDeviceAddress[256];
char pairData[16384];
uint32_t pairDataLen;

struct UPNPUrls *upnpUrls;
struct IGDdatas *upnpData;
char upnpExternalAddr[40];
idevice_t g_device;

#ifdef __cplusplus
}
#endif
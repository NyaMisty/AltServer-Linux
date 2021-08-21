#include "common.h"
#define main tool_main
#define idevice_new_with_options wrap_idevice_new_with_options
idevice_error_t wrap_idevice_new_with_options(idevice_t *device, const char *udid, enum idevice_options options) {
    *device = g_device;
    return IDEVICE_E_SUCCESS;
}
#define PACKAGE_NAME "-"
#define PACKAGE_VERSION "-"
#define PACKAGE_URL "-"
#define PACKAGE_BUGREPORT "-"
// #include <../libraries/libimobiledevice/tools/ideviceinfo.c>
#include <../libraries/ideviceinstaller/src/ideviceinstaller.c>
#undef main
#undef idevice_new_with_options
#undef lockdownd_client_new_with_handshake

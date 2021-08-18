#include "libusbmuxd-stub.h"
#include <unistd.h>
#include <pthread.h>

#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/debugserver.h>
#include <libimobiledevice/heartbeat.h>
#include <libimobiledevice/installation_proxy.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/mobile_image_mounter.h>
#include <libimobiledevice/sbservices.h>
#include <libimobiledevice/service.h>
#include "common/userpref.h"
#include "common/utils.h"

idevice_t g_device = NULL;

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


#ifndef TOOL_NAME
#define TOOL_NAME "jitterbug1"
#endif

static void heartbeat_thread(heartbeat_client_t client) {
    while (1) {
        plist_t ping;
        uint64_t interval = 15;
        //DEBUG_PRINT("Timer run!");
        if (heartbeat_receive_with_timeout(client, &ping, (uint32_t)interval * 1000) != HEARTBEAT_E_SUCCESS) {
            DEBUG_PRINT("Did not recieve ping, canceling timer!");
            return;
        }
        plist_get_uint_val(plist_dict_get_item(ping, "Interval"), &interval);
        //DEBUG_PRINT("Set new timer interval: %lu!", interval);
        //DEBUG_PRINT("Sending heartbeat.");
        heartbeat_send(client, ping);
        plist_free(ping);
        sleep(interval);
    }
}


lockdownd_client_t g_lockdown = NULL;

// #define lockdownd_client_new_with_handshake wrap_lockdownd_client_new_with_handshake
// lockdownd_error_t wrap_lockdownd_client_new_with_handshake(idevice_t device, lockdownd_client_t *client, const char *label) {
//     *client = g_lockdown;
//     return LOCKDOWN_E_SUCCESS;
// }

int main(int argc, char *argv[]) {
    DEBUG_PRINT("Setup pairInfo...");
    strcpy(pairUDID, argv[1]);
    strcpy(pairDeviceAddress, argv[2]);
    FILE *f = fopen(argv[3], "rb");
    fseek(f, 0, SEEK_END);
    pairDataLen = ftell(f);
    fseek(f, 0, SEEK_SET);
    fread(pairData, 1, pairDataLen, f);

    DEBUG_PRINT("Connect device...");
    idevice_error_t derr = IDEVICE_E_SUCCESS;
    lockdownd_error_t lerr = LOCKDOWN_E_SUCCESS;
    if ((derr = idevice_new_with_options(&g_device, pairUDID, IDEVICE_LOOKUP_NETWORK)) != IDEVICE_E_SUCCESS) {
        DEBUG_PRINT("Failed to create device: %d", derr);
        return 1;
    }
    
    if ((lerr = lockdownd_client_new_with_handshake(g_device, &g_lockdown, TOOL_NAME)) != LOCKDOWN_E_SUCCESS) {
        DEBUG_PRINT("Failed to communicate with device: %d", lerr);
        return 1;
    }
    
    DEBUG_PRINT("Start hb...");
    heartbeat_client_t hbclient;
    heartbeat_error_t err = HEARTBEAT_E_UNKNOWN_ERROR;
    err = heartbeat_client_start_service(g_device, &hbclient, TOOL_NAME);
    
    pthread_t thHeartbeat;
    pthread_create(&thHeartbeat, NULL, (void * (*)(void *))heartbeat_thread, hbclient);

    //idevice_set_debug_level(1);
    DEBUG_PRINT("Start tool...");
    tool_main(argc - 3, argv + 3);
}
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

//#define TOOL_NAME "jitterbug1"

static service_error_t service_client_factory_start_service_with_lockdown(lockdownd_client_t lckd, idevice_t device, const char* service_name, void **client, const char* label, int32_t (*constructor_func)(idevice_t, lockdownd_service_descriptor_t, void**), int32_t *error_code)
{
    *client = NULL;

    lockdownd_service_descriptor_t service = NULL;
    lockdownd_start_service(lckd, service_name, &service);

    if (!service || service->port == 0) {
        DEBUG_PRINT("Could not start service %s!", service_name);
        return SERVICE_E_START_SERVICE_ERROR;
    }

    int32_t ec;
    if (constructor_func) {
        ec = (int32_t)constructor_func(device, service, client);
    } else {
        ec = service_client_new(device, service, (service_client_t*)client);
    }
    if (error_code) {
        *error_code = ec;
    }

    if (ec != SERVICE_E_SUCCESS) {
        DEBUG_PRINT("Could not connect to service %s! Port: %i, error: %i", service_name, service->port, ec);
    }

    lockdownd_service_descriptor_free(service);
    service = NULL;

    return (ec == SERVICE_E_SUCCESS) ? SERVICE_E_SUCCESS : SERVICE_E_START_SERVICE_ERROR;
}

static void heartbeat_thread(heartbeat_client_t client) {
    while (1) {
        plist_t ping;
        uint64_t interval = 15;
        DEBUG_PRINT("Timer run!");
        if (heartbeat_receive_with_timeout(client, &ping, (uint32_t)interval * 1000) != HEARTBEAT_E_SUCCESS) {
            DEBUG_PRINT("Did not recieve ping, canceling timer!");
            return;
        }
        plist_get_uint_val(plist_dict_get_item(ping, "Interval"), &interval);
        DEBUG_PRINT("Set new timer interval: %llu!", interval);
        DEBUG_PRINT("Sending heartbeat.");
        heartbeat_send(client, ping);
        plist_free(ping);
        sleep(interval * 1000);
    }
}


idevice_t g_device = NULL;
lockdownd_client_t g_lockdown = NULL;

#define main tool_main
#define idevice_new_with_options wrap_idevice_new_with_options
#define lockdownd_client_new_with_handshake wrap_lockdownd_client_new_with_handshake
idevice_error_t wrap_idevice_new_with_options(idevice_t *device, const char *udid, enum idevice_options options) {
    *device = g_device;
    return IDEVICE_E_SUCCESS;
}

lockdownd_error_t wrap_lockdownd_client_new_with_handshake(idevice_t device, lockdownd_client_t *client, const char *label) {
    *client = g_lockdown;
    return LOCKDOWN_E_SUCCESS;
}
#define PACKAGE_VERSION "-"
#define PACKAGE_URL "-"
#define PACKAGE_BUGREPORT "-"
#include <../libraries/libimobiledevice/tools/ideviceinfo.c>
#undef main
#undef idevice_new_with_options
#undef lockdownd_client_new_with_handshake


int main(int argc, char *argv[]) {
    DEBUG_PRINT("Setup pairInfo...");
    strcpy(pairUDID, argv[1]);
    strcpy(pairDeviceAddress, argv[2]);
    FILE *f = fopen(argv[3], "rb");
    fseek(f, 0, SEEK_END);
    long pairDataLen = ftell(f);
    fseek(f, 0, SEEK_SET);
    fread(pairData, 1, pairDataLen, f);
    
    
    idevice_error_t derr = IDEVICE_E_SUCCESS;
    lockdownd_error_t lerr = LOCKDOWN_E_SUCCESS;
    if ((derr = idevice_new_with_options(&g_device, pairUDID, IDEVICE_LOOKUP_NETWORK | IDEVICE_LOOKUP_USBMUX)) != IDEVICE_E_SUCCESS) {
        DEBUG_PRINT("Failed to create device: %d", derr);
        return 1;
    }
    
    if ((lerr = lockdownd_client_new_with_handshake(g_device, &g_lockdown, TOOL_NAME)) != LOCKDOWN_E_SUCCESS) {
        DEBUG_PRINT("Failed to communicate with device: %d", lerr);
        return 1;
    }
    
    heartbeat_client_t client;
    heartbeat_error_t err = HEARTBEAT_E_UNKNOWN_ERROR;
    service_client_factory_start_service_with_lockdown(g_lockdown, g_device, HEARTBEAT_SERVICE_NAME, (void **)&client, TOOL_NAME, SERVICE_CONSTRUCTOR(heartbeat_client_new), &err);
    
    pthread_t thHeartbeat;
    pthread_create(&thHeartbeat, NULL, (void * (*)(void *))heartbeat_thread, client);
    
    tool_main(1, (char *[]){""});
}
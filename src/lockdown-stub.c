#define lockdownd_client_new_with_handshake wrap_lockdownd_client_new_with_handshake
#include "../libraries/libimobiledevice/src/lockdown.c"
#undef lockdownd_client_new_with_handshake

#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/heartbeat.h>
#include <libimobiledevice/sbservices.h>
#include <libimobiledevice/service.h>

#define DEBUG_PRINT(...) do { \
        fprintf(stderr, "[%s:%d] ", __FUNCTION__, __LINE__); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } while (0)


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
        //DEBUG_PRINT("Timer run!");
        if (heartbeat_receive_with_timeout(client, &ping, (uint32_t)interval * 1000) != HEARTBEAT_E_SUCCESS) {
            DEBUG_PRINT("Did not recieve ping, canceling timer!");
            return;
        }
        plist_get_uint_val(plist_dict_get_item(ping, "Interval"), &interval);
        DEBUG_PRINT("Set new timer interval: %lu!", interval);
        //DEBUG_PRINT("Sending heartbeat.");
        heartbeat_send(client, ping);
        plist_free(ping);
        sleep(interval);
    }
}


LIBIMOBILEDEVICE_API lockdownd_error_t lockdownd_client_new_with_handshake(idevice_t device, lockdownd_client_t *client, const char *label) {
    lockdownd_error_t ret = wrap_lockdownd_client_new_with_handshake(device, client, label);
    if (ret == LOCKDOWN_E_SUCCESS) {
        //DEBUG_PRINT("Start hb...");
        // heartbeat_client_t hb_client;
        // heartbeat_error_t err = HEARTBEAT_E_UNKNOWN_ERROR;
        // service_client_factory_start_service_with_lockdown(*client, device, HEARTBEAT_SERVICE_NAME, (void **)&hb_client, label, SERVICE_CONSTRUCTOR(heartbeat_client_new), &err);
        
        // pthread_t thHeartbeat;
        // pthread_create(&thHeartbeat, NULL, (void * (*)(void *))heartbeat_thread, hb_client);
    }
    
    return ret;
}
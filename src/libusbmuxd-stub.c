//
// Copyright © 2021 osy. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#define USBMUXD_API __attribute__((visibility("default")))

// usbmuxd public interface
#include "usbmuxd.h"
// usbmuxd protocol
#include "usbmuxd-proto.h"
// custom functions
#include "common/userpref.h"
//#include "CacheStorage.h"
//#include "Jitterbug.h"

#include "libusbmuxd-stub.h"

char pairUDID[256] = { 0 };
char pairDeviceAddress[256] = { 0 };
char pairData[8192] = { 0 };
uint32_t pairDataLen = 0;



#pragma mark - Device listing

struct sockaddr_in_darwin {
    uint8_t      sin_len;
    sa_family_t    sin_family;
    in_port_t      sin_port;
    struct in_addr sin_addr;
    char           sin_zero[8];
};

USBMUXD_API int usbmuxd_get_device_by_udid(const char *udid, usbmuxd_device_info_t *device)
{
    if (!udid) {
        DEBUG_PRINT("udid cannot be null!");
        return -EINVAL;
    }
    if (!device) {
        DEBUG_PRINT("device cannot be null!");
        return -EINVAL;
    }
    struct sockaddr_in_darwin addr = {0};
    addr.sin_len = sizeof(addr);
    addr.sin_family = AF_INET;
    inet_aton(pairDeviceAddress, &addr.sin_addr);
    strcpy(device->udid, pairUDID);
    memcpy(device->conn_data, &addr, sizeof(struct sockaddr_in_darwin));
    device->conn_type = CONNECTION_TYPE_NETWORK;
    return 1;
}

USBMUXD_API int usbmuxd_get_device(const char *udid, usbmuxd_device_info_t *device, enum usbmux_lookup_options options)
{
    if ((options & DEVICE_LOOKUP_USBMUX) != 0) {
        DEBUG_PRINT("DEVICE_LOOKUP_USBMUX not supported!");
        return -EINVAL;
    } else {
        return usbmuxd_get_device_by_udid(udid, device);
    }
}

#pragma mark - Device pairing

USBMUXD_API int usbmuxd_read_buid(char **buid)
{
    // ignore BUID
    return -EINVAL;
}

USBMUXD_API int usbmuxd_read_pair_record(const char* record_id, char **record_data, uint32_t *record_size)
{
    void *data = pairData;
    size_t len = pairDataLen;
    *record_data = data;
    *record_size = (uint32_t)len;
    return 0;
}

#pragma mark - Unimplemented functions

USBMUXD_API int usbmuxd_events_subscribe(usbmuxd_subscription_context_t *context, usbmuxd_event_cb_t callback, void *user_data)
{
    abort();
}

USBMUXD_API int usbmuxd_events_unsubscribe(usbmuxd_subscription_context_t context)
{
    abort();
}

USBMUXD_API int usbmuxd_get_device_list(usbmuxd_device_info_t **device_list)
{
    abort();
}

USBMUXD_API int usbmuxd_device_list_free(usbmuxd_device_info_t **device_list)
{
    abort();
}

USBMUXD_API int usbmuxd_subscribe(usbmuxd_event_cb_t callback, void *user_data)
{
    abort();
}

USBMUXD_API int usbmuxd_unsubscribe(void)
{
    abort();
}

USBMUXD_API int usbmuxd_connect(const uint32_t handle, const unsigned short port)
{
    abort();
}

USBMUXD_API int usbmuxd_disconnect(int sfd)
{
    abort();
}

USBMUXD_API int usbmuxd_send(int sfd, const char *data, uint32_t len, uint32_t *sent_bytes)
{
    abort();
}

USBMUXD_API int usbmuxd_recv_timeout(int sfd, char *data, uint32_t len, uint32_t *recv_bytes, unsigned int timeout)
{
    abort();
}

USBMUXD_API int usbmuxd_recv(int sfd, char *data, uint32_t len, uint32_t *recv_bytes)
{
    abort();
}

USBMUXD_API int usbmuxd_save_pair_record_with_device_id(const char* record_id, uint32_t device_id, const char *record_data, uint32_t record_size)
{
    abort();
}

USBMUXD_API int usbmuxd_save_pair_record(const char* record_id, const char *record_data, uint32_t record_size)
{
    abort();
}

USBMUXD_API int usbmuxd_delete_pair_record(const char* record_id)
{
    abort();
}

USBMUXD_API void libusbmuxd_set_use_inotify(int set)
{
    abort();
}

USBMUXD_API void libusbmuxd_set_debug_level(int level)
{
    abort();
}


#include "../common.h"
#include "global.h"

#define idevice_connect wrap_idevice_connect
#include "../libraries/libimobiledevice/src/idevice.c"
#undef idevice_connect

#include "miniwget.h"
#include "miniupnpc.h"
#include "upnpcommands.h"
#include "portlistingparse.h"
#include "upnperrors.h"
#include <stdlib.h>
#include <arpa/inet.h>

static int add_upnp_redir(const char *addr, int port, int *retPort) {
    char *proto = "TCP";
    char *description = "altserver";
    char *lease = "600";
    char iport[6] = {0};
    sprintf(iport, "%d", port);
    int minport = 16384;
    int maxport = 65536;
    
    for (int i = 0; i < 5; i++) {
        int curport = rand() % (maxport - minport) + minport;
        char eport[6] = {0};
        sprintf(eport, "%d", curport);
        int r = UPNP_AddPortMapping(upnpUrls->controlURL, upnpData->first.servicetype,
                    eport, iport, addr, description,
                    proto, NULL, lease);
        if(r==UPNPCOMMAND_SUCCESS) {
            *retPort = curport;
            goto good;
        } else {
            printf("AddAnyPortMapping failed with code %d (%s), retrying...\n",
                    r, strupnperror(r));
            continue;
        }
    }
    return 0;
    good:
	return 1;
}

LIBIMOBILEDEVICE_API idevice_error_t idevice_connect(idevice_t device, uint16_t port, idevice_connection_t *connection) {
    if (device->conn_type == CONNECTION_NETWORK) {
        int redirPort = 0;
        if (!add_upnp_redir(pairDeviceAddress, port, &redirPort)) {
            DEBUG_PRINT("failed to add upnp port map!");
            return IDEVICE_E_SSL_ERROR;
        }
		
		debug_info("Connecting to %s:%d via %s:%d...", pairDeviceAddress, port, upnpExternalAddr, redirPort);

        struct sockaddr_storage saddr_storage;
		struct sockaddr_in* saddr = (struct sockaddr_in*)&saddr_storage;
        saddr->sin_family = AF_INET;
        inet_aton(upnpExternalAddr, &saddr->sin_addr);

		int sfd = socket_connect_addr((struct sockaddr *)saddr, redirPort);
		if (sfd < 0) {
			debug_info("ERROR: Connecting to network device failed: %d (%s)", errno, strerror(errno));
			return IDEVICE_E_NO_DEVICE;
		}

		idevice_connection_t new_connection = (idevice_connection_t)malloc(sizeof(struct idevice_connection_private));
		new_connection->type = CONNECTION_NETWORK;
		new_connection->data = (void*)(long)sfd;
		new_connection->ssl_data = NULL;
		new_connection->device = device;
		new_connection->ssl_recv_timeout = (unsigned int)-1;

		*connection = new_connection;

		return IDEVICE_E_SUCCESS;
    } else {
        return wrap_idevice_connect(device, port, connection);
    }
}
#include <sys/socket.h>
#include <arpa/inet.h>

#define SOCKADDR struct sockaddr
#define WSAGetLastError() errno
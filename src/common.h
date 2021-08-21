#pragma once

#include <stdint.h>

typedef int HWND;
typedef int HINSTANCE;

#define DEBUG_PRINT(...) do { \
        fprintf(stderr, "[%s:%d] ", __FUNCTION__, __LINE__); \
        fprintf(stderr, __VA_ARGS__); \
        fprintf(stderr, "\n"); \
    } while (0)

#include <stdlib.h>
#include <unistd.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#define closesocket close
typedef struct timeval TIMEVAL;
typedef int BOOL;

#ifndef odslog
#define odslog(msg) { std::cout << msg << std::endl; }
#endif


#define OutputDebugStringW(x) (std::wcout << x)
#define OutputDebugStringA(x) (std::cout << x)
cmake_minimum_required(VERSION 3.4.1...3.17.2)

project(AltServer)

set (CMAKE_CXX_STANDARD 17)
set (CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)

set(NO_USBMUXD_STUB ON CACHE BOOL "NO_USBMUXD_STUB Option")

if (UNIX)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_CONFIG_H -DDEBUG -O0 -g")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHAVE_CONFIG_H -DDEBUG -O0 -g")
endif()

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif()

FILE(GLOB main_src src/*.c src/*.cpp)

# NOTE: remove this file, or it will lead to stack overflow caused by recursive `memory'
if (APPLE)
    list(REMOVE_ITEM main_src ${CMAKE_SOURCE_DIR}/src/muslfix.c)
endif()

FILE(GLOB libimobiledevice_src
    libraries/libimobiledevice/src/*.c
    libraries/libimobiledevice/common/*.c)

message(STATUS "NO_USBMUXD_STUB: " ${NO_USBMUXD_STUB})
if (NO_USBMUXD_STUB)
    add_definitions(-DNO_USBMUXD_STUB)
    FILE(GLOB libusbmuxd_src
        libraries/libusbmuxd/src/*.c
        libraries/libusbmuxd/common/collection.c)
    list(APPEND libimobiledevice_src ${libusbmuxd_src})
else()
    list(APPEND main_src src/phone/libusbmuxd-stub.c)
    if (NO_UPNP_STUB)
        add_definitions(-DNO_UPNP_STUB)
    else()
        list(REMOVE_ITEM libimobiledevice_src ${CMAKE_SOURCE_DIR}/libraries/libimobiledevice/src/idevice.c)
        list(APPEND main_src src/phone/idevice-stub.c)
    endif()
endif()

# libplist
FILE(GLOB libplist_src libraries/libplist/src/*.c)
list(APPEND libplist_src
            libraries/libplist/libcnary/node.c
            libraries/libplist/libcnary/node_list.c)


# miniupnpc_src
set(miniupnpc_src
    libraries/miniupnpc/minissdpc.c
    libraries/miniupnpc/miniwget.c
    libraries/miniupnpc/minixml.c
    libraries/miniupnpc/igd_desc_parse.c
    libraries/miniupnpc/minisoap.c
    libraries/miniupnpc/miniupnpc.c
    libraries/miniupnpc/upnpreplyparse.c
    libraries/miniupnpc/upnpcommands.c
    libraries/miniupnpc/upnperrors.c
    libraries/miniupnpc/connecthostport.c
    libraries/miniupnpc/portlistingparse.c
    libraries/miniupnpc/receivedata.c
    libraries/miniupnpc/upnpdev.c
	libraries/miniupnpc/addr_is_reserved.c)

# for corecrypto
if (APPLE)
    list(APPEND main_src libraries/corecrypto/Sources/ccsrp.m)
endif()

add_executable(AltServer
    ${main_src}
    ${libimobiledevice_src}
    ${libplist_src}
    ${miniupnpc_src}
)

# for corecrypto
if (APPLE)
    add_definitions(-DCORECRYPTO_DONOT_USE_TRANSPARENT_UNION=1)
endif()

target_include_directories(AltServer PUBLIC
    ${OPENSSL_INCLUDE_DIR}
    libraries
    libraries/AltSign
    libraries/libimobiledevice
    libraries/libimobiledevice/include
    libraries/libusbmuxd/include
    libraries/libplist/include
    libraries/libplist/libcnary/include
    libraries/libplist/src
    libraries/libusbmuxd/common
    )

add_subdirectory(libraries/AltSign)

if (UNIX)
  set(THREADS_PREFER_PTHREAD_FLAG TRUE)
  find_package(Threads)
  target_link_libraries(AltServer PRIVATE Threads::Threads)
endif()

target_link_libraries(AltServer PRIVATE AltSign ssl crypto z cpprest boost_system boost_filesystem uuid c++)

# Help finding Homebrew's OpenSSL on macOS
if (APPLE)
    # This is for apple silicon (M1)
    set(HOMEBREW_PREFIX "/opt/homebrew"
        CACHE PATH "Path to Homebrew installation")

    # This is for apple x86
    # set(HOMEBREW_PREFIX "/usr/local"
    #     CACHE PATH "Path to Homebrew installation")

    set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${HOMEBREW_PREFIX}/opt/openssl/lib)
    set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} ${HOMEBREW_PREFIX}/opt/openssl/include)

    # This OPENSSL_FOUND check is to help find a cmake manually configured OpenSSL
    if (NOT OPENSSL_FOUND)
        find_package(OpenSSL REQUIRED)
    endif()
    message(STATUS "OpenSSL: " ${OPENSSL_VERSION} ${OPENSSL_INCLUDE_PATH})

    add_definitions(${OPENSSL_DEFINITIONS})
    target_include_directories(AltServer PUBLIC
            ${HOMEBREW_PREFIX}/include
            ${OPENSSL_INCLUDE_DIR}
            libraries/corecrypto/include/corecrypto
            )

    target_link_directories(AltServer PUBLIC
            ${HOMEBREW_PREFIX}/opt/openssl/lib
            ${HOMEBREW_PREFIX}/lib
            )
    target_link_libraries(AltServer PRIVATE "-framework Foundation")
endif()

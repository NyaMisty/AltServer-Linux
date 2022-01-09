# AltServer-Linux
AltServer for AltStore, but on-device

## Usage

- For build configuration 1 (AltServer): Works just like a normal AltServer on windows
  - Install IPA: `./AltServer -u [UDID] -a [AppleID account] -p [AppleID password] [ipaPath.ipa]`
  - Running as AltServer Daemon: `./AltServer`
- For build configuration 2 (AltServerNet): AltServer over Network
  - Install IPA: `./AltServerNet -u [UDID] -P [jitterbug pair file] -i [device IP] -a [AppleID account] -p [AppleID password] [ipaPath.ipa]`
  - Running as AltServer Daemon not supported
- For build configuration 2 (AltServerUPnP): AltServer over Network
  - Install IPA: `./AltServerUPnP -u [UDID] -P [jitterbug pair file] -i [device IP] -a [AppleID account] -p [AppleID password] [ipaPath.ipa]`

## Build Instruction

- Preparation: `git clone --recursive https://github.com/NyaMisty/AltServer-Linux`

- Install corecrypto_static, cpprestsdk static lib, boost static lib

- Build Configuration 1 (AltServer): Simple AltServer running on normal Linux host, working through usbmuxd
  ```
  make NO_USBMUXD_STUB=1 NO_UPNP_STUB=1
  ```
  
- Build Configuration 2 (AltServerNet): AltServer running on normal Linux host, working through manually specifying IP
  ```
  make NO_UPNP_STUB=1
  ```

- Build Configuration 3 (AltServerUPnP): AltServer running on iSH, working through manually specifying IP in a UPnP enabled router
  ```
  make
  ```

- My own build note for you 
  ```
    1. Run alpine docker (change --platform to corresponding architecture you want): 
        docker run --platform=linux/arm/v7 --name altserver-builder-alpine-armv7 -it alpine:3.15 
    2. Install dependencies:
        apk add zsh git curl wget g++ clang boost-static ninja boost-dev cmake make sudo bash vim libressl-dev util-linux-dev zlib-dev zlib-static
    3. Install corecrypto
        download corecrypto from apple website, unzip corecrypto.zip; cd corecrypto; mkdir build; cd build; CC=clang CXX=clang++ cmake ..;
        vim CMakeFiles/Makefile2, delete line starts with "all: corecrypto_perf/....." and "all: corecrypto_test/.....", then make; make install
    4. Install cpprestsdk
        git clone --recursive https://github.com/microsoft/cpprestsdk; cd cpprestsdk; mkdir build; cmake -DBUILD_SHARED_LIBS=0 ..; make; make install
	    (if you're compiling for armv7, you have to grep -Wcast-align, and remove it, or the compiling would fail)
    5. Install libzip
        git clone https://github.com/nih-at/libzip; cd libzip; mkdir build; cd build; cmake -DBUILD_SHARED_LIBS=0 ..; make; make install
    6. Compile AltServer-Linux
        git clone --recursive https://github.com/NyaMisty/AltServer-Linux
        make NO_USBMUXD_STUB=1 NO_UPNP_STUB=1
	    (if you're compiling for ARM, i.e. armv7 or aarch64, you'll have to remove the -mno-default flag in Makefile)

  ```

## TODO

- [ ] Support Wi-Fi Refresh

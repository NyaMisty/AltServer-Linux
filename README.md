# AltServer-Linux

AltServer for AltStore, but on-device

## Usage

- 📦 Install IPA
  ```bash
  ./AltServer -u [UDID] -a [AppleID account] -p [AppleID password] [ipaPath.ipa]
  ```

- ➰ Running as AltServer Daemon 
  ```bash
  ./AltServer
  ```

- 🧪 Full Usage
  ```bash
  Usage:  AltServer-Linux options [ ipa-file ]
    -h  --help             Display this usage information.
    -u  --udid UDID        Device's UDID, only needed when installing IPA.
    -a  --appleID AppleID  Apple ID to sign the ipa, only needed when installing IPA.
    -p  --password passwd  Password of Apple ID, only needed when installing IPA.
    -d  --debug            Print debug output, can be used several times to increase debug level.

  The following environment var can be set for some special situation:
    - ALTSERVER_ANISETTE_SERVER: Set to custom anisette server URL
            if not set, the default one: https://armconverter.com/anisette/irGb3Quww8zrhgqnzmrx, is used
    - ALTSERVER_NO_SUBSCRIBE: (*unused*) Please enable this for usbmuxd server that do not correctly usbmuxd_listen interfaces
  ```

## Download

- Precompiled static binary can be downloaded in Release ( also have a look at pre-release ;) )
- Nightly version is available as Github Actions artifacts

## TODO / Special Features
- [x] Track upstream (AltServer-Windows) develop branch (i.e. Beta version)
- [x] Support Offline Anisette Data Generation (i.e. without Sideloadly)
  - Finsihed, please run [alt_anisette_server](https://hub.docker.com/r/nyamisty/alt_anisette_server) & use `ALTSERVER_ANISETTE_SERVER` to specify custom server URL
- [x] Support Wi-Fi Refresh
  - [netmuxd](https://github.com/jkcoxson/netmuxd) now supports network devices (needs version > v0.1.1, be sure to check pre-release)
    - Download `netmuxd`, stop the original `usbmuxd`, and run `netmuxd` before running `AltServer-Linux`
    - ~If netmuxd does not work, please try using special env var `ALTSERVER_NO_SUBSCRIBE`. Enabling this would disable **auto-refresh when plugged-in** of USB devices~

----

## Advanced: Build Instructions
> If you have issues with building you may want to checkout the build workflow

> **Note**  
> The following build instructions apply to archlinux only, but should be roughly the same on other distros.

- 🚧 Install Build Dependencies
  ```bash
  yay -S boost clang cmake ninja git libcorecrypto-git zlib python3 avahi
  ```

- 📦 Grab the source
  ```bash
  git clone --recursive https://github.com/NyaMisty/AltServer-Linux
  cd AltServer-Linux
  ````

- 🏗️ Build
  ```bash
  mkdir build && cd build
  cmake .. && cmake --build . --config Release
  ```
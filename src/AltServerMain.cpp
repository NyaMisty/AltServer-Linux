// HelloWindowsDesktop.cpp
// compile with: /D_UNICODE /DUNICODE /DWIN32 /D_WINDOWS /c

#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <fstream>
#include <iterator>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <codecvt>
#include <random>

#define _T(x) x

// AltSign
#include "DeviceManager.hpp"
#include "Error.hpp"

#include "AltServerApp.h"

//#include "PhoneHelper.h"

#define odslog(msg) { std::stringstream ss; ss << msg << std::endl; OutputDebugStringA(ss.str().c_str()); }

#include <pplx/pplxtasks.h>
#include <pplx/threadpool.h>

#include <uuid/uuid.h>
std::string make_uuid() {
    uuid_t b;
	char out[UUID_STR_LEN] = {0};
	uuid_generate(b);
  	uuid_unparse_lower(b, out);
	return out;
}

std::string temporary_directory()
{
	return fs::temp_directory_path().string();
}

std::vector<unsigned char> readFile(const char* filename)
{
	// open the file:
	std::ifstream file(filename, std::ios::binary);

	// Stop eating new lines in binary mode!!!
	file.unsetf(std::ios::skipws);

	// get its size:
	std::streampos fileSize;

	file.seekg(0, std::ios::end);
	fileSize = file.tellg();
	file.seekg(0, std::ios::beg);

	// reserve capacity
	std::vector<unsigned char> vec;
	vec.reserve(fileSize);

	// read the data:
	vec.insert(vec.begin(),
		std::istream_iterator<unsigned char>(file),
		std::istream_iterator<unsigned char>());

	return vec;
}

#ifndef NO_USBMUXD_STUB
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

boost::asio::io_service io_service;
std::shared_ptr<boost::asio::steady_timer> timer;
void *hbclient;

void heartbeat_tick(const boost::system::error_code& /*e*/) {
	int intervalSec = do_heartbeat(hbclient);
    odslog("heartbeat_tick! interval: " << intervalSec);
	if (!intervalSec) {
		return;
	}
	// Reschedule the timer for 1 second in the future:
    //timer->expires_from_now(boost::posix_time::seconds(4));
    // Posts the timer event
    //timer->async_wait(heartbeat_tick);

	auto &ioService = crossplat::threadpool::shared_instance().service();
	boost::asio::steady_timer t(ioService, std::chrono::seconds(4)); 
	t.async_wait(heartbeat_tick);
}

int setupHeartbeatTimer() {
	auto &ioService = crossplat::threadpool::shared_instance().service();

    // timer = std::make_shared<boost::asio::steady_timer>(ioService);
    // timer->expires_from_now(boost::posix_time::seconds(1));
    // timer->async_wait(heartbeat_tick);
	
	boost::system::error_code err;
	heartbeat_tick(err);
	return 1;
}
#endif

#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED
#include <boost/stacktrace.hpp>

int main(int argc, char *argv[]) {
	static struct option long_options[] =
        {
          {"udid",		required_argument,   	0, 'u'},
          {"ipaddr",	required_argument,		0, 'i'},
          {"appleID",	required_argument,      0, 'a'},
          {"password",	required_argument,      0, 'p'},
		  {"pairData",	required_argument,      0, 'P'},
		  {"debug",		no_argument,      		0, 'd'},
          {0, 0, 0, 0}
        };
	
	char *udid;
	char *ipaddr;
	char *appleID;
	char *password;
	char *pairDataFile;
	
	char *ipaPath = NULL;
	bool debugLog = false;

	while (1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;

		int c = getopt_long (argc, argv, "u:i:a:p:P:d",
						long_options, &option_index);
		if (c == -1) break;

		switch (c) {
        case 'u':
			udid = optarg;
            break;
       	case 'i':
			ipaddr = optarg;
			break;
        case 'a':
			appleID = optarg;
        case 'p':
            password = optarg;
			break;
		case 'P':
            pairDataFile = optarg;
			break;
		case 'd':
			debugLog = true;
			break;
       	default:
            printf("?? getopt returned character code 0%o ??\n", c);
    	}
	}

	bool installApp = true;
	if (optind == argc) {
		printf("Not supplying ipa, running in server mode!\n");
        installApp = false;
    } else if (optind + 1 == argc) {
		ipaPath = argv[optind];
	} else {
		printf("Unknown options: ");
 		while (optind < argc)
            printf("%s ", argv[optind++]);
        printf("\n");
		return 1;
	}

	setvbuf(stdin, NULL, _IONBF, 0); 
    setvbuf(stdout, NULL, _IONBF, 0); 
    setvbuf(stderr, NULL, _IONBF, 0); 
	
	srand(time(NULL));

	if (debugLog) {
		idevice_set_debug_level(1);
	}

    
#ifndef NO_USBMUXD_STUB
    setupPairInfo(udid, ipaddr, pairDataFile);

#ifndef NO_UPNP_STUB
	if (!initUPnP()) {
        DEBUG_PRINT("failed to init upnp! exitting...");
        return 1;
    }
    DEBUG_PRINT("upnp init successfully!");    
#endif

	DEBUG_PRINT("Connect device...");
	if (!initGlobalDevice()) {
		DEBUG_PRINT("failed to init device! exitting...");
		return 1;
	}
    if (!initHeartbeat(&hbclient)) {
		DEBUG_PRINT("failed to init heartbeat! exitting...");
		return 1;
	}
    if (!setupHeartbeatTimer()) {
        DEBUG_PRINT("failed to init heartbeat! exitting...");
        return 1;
    }
	DEBUG_PRINT("heartbeat init successfully!");    

#endif

	signal(SIGPIPE, SIG_IGN);

	if (installApp) {
		odslog("Installing app...");
		std::shared_ptr<Device> _selectedDevice = std::make_shared<Device>("unknown", udid, Device::Type::All);;
		std::optional<std::string> _ipaFilepath = std::make_optional<std::string>(ipaPath);
		auto task = AltServerApp::instance()->InstallApplication(_ipaFilepath, _selectedDevice, (appleID), (password));
		try
		{
			task.get();
		}
		catch (Error& error)
		{
			odslog("Error: " << error.domain() << " (" << error.code() << ").")
		}
		catch (std::exception& exception)
		{
			odslog("Exception: " << exception.what());
			odslog(boost::stacktrace::stacktrace());
		}

		odslog("Finished!");
	} else {
		AltServerApp::instance()->Start(0, 0);
		while (1) {
			sleep(100);
		}
	}
}
#pragma once

#include "common.h"
#include "Device.hpp"

#include <pplx/pplxtasks.h>
#include <cpprest/json.h>

#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/notification_proxy.h>

#include <memory>
#include <set>

class ClientConnection
{
public:
	ClientConnection();
	virtual ~ClientConnection();

	virtual void Disconnect();

	pplx::task<void> ProcessAppRequest();

	pplx::task<void> ProcessPrepareAppRequest(web::json::value request);
	pplx::task<void> ProcessAnisetteDataRequest(web::json::value request);
	pplx::task<void> ProcessInstallProfilesRequest(web::json::value request);
	pplx::task<void> ProcessRemoveProfilesRequest(web::json::value request);
	pplx::task<void> ProcessRemoveAppRequest(web::json::value request);

	pplx::task<void> SendResponse(web::json::value json);
	pplx::task<web::json::value> ReceiveRequest();

	virtual pplx::task<void> SendData(std::vector<unsigned char>& data) = 0;
	virtual pplx::task<std::vector<unsigned char>> ReceiveData(int size) = 0;

private:
	pplx::task<std::string> ReceiveApp(web::json::value request);
	pplx::task<void> InstallApp(std::string filepath, std::string udid, std::optional<std::set<std::string>> activeProfiles);

	web::json::value ErrorResponse(std::exception& exception);
};


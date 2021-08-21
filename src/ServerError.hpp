//
//  ServerError.hpp
//  AltServer-Windows
//
//  Created by Riley Testut on 8/13/19.
//  Copyright © 2019 Riley Testut. All rights reserved.
//

#ifndef ServerError_hpp
#define ServerError_hpp

#include "Error.hpp"

extern std::string LocalizedFailureErrorKey;
extern std::string UnderlyingErrorCodeErrorKey;
extern std::string ProvisioningProfileBundleIDErrorKey;

enum class ServerErrorCode
{
	UnderlyingError = -1,

    Unknown = 0,
    ConnectionFailed = 1,
    LostConnection = 2,
    
    DeviceNotFound = 3,
    DeviceWriteFailed = 4,
    
    InvalidRequest = 5,
    InvalidResponse = 6,
    
    InvalidApp = 7,
    InstallationFailed = 8,
    MaximumFreeAppLimitReached = 9,
	UnsupportediOSVersion = 10,

	UnknownRequest = 11,
	UnknownResponse = 12,

	InvalidAnisetteData = 13,
	PluginNotFound = 14,

	ProfileNotFound = 15,

	AppDeletionFailed = 16
};

class ServerError: public Error
{
public:
    ServerError(ServerErrorCode code) : Error((int)code)
    {
    }

	ServerError(ServerErrorCode code, std::map<std::string, std::string> userInfo) : Error((int)code, userInfo)
	{
	}
    
    virtual std::string domain() const
    {
        return "com.rileytestut.AltServer";
    }
    
    virtual std::string localizedDescription() const
    {
		switch ((ServerErrorCode)this->code())
		{
		case ServerErrorCode::UnderlyingError:
		case ServerErrorCode::Unknown:
			return "An unknown error occured.";

		case ServerErrorCode::ConnectionFailed:
			return "Could not connect to device.";

		case ServerErrorCode::LostConnection:
			return "Lost connection to AltServer.";

		case ServerErrorCode::DeviceNotFound:
			return "AltServer could not find the device.";

		case ServerErrorCode::DeviceWriteFailed:
			return "Failed to write app data to device.";

		case ServerErrorCode::InvalidRequest:
			return "AltServer received an invalid request.";

		case ServerErrorCode::InvalidResponse:
			return "AltServer sent an invalid response.";

		case ServerErrorCode::InvalidApp:
			return "The app is invalid.";

		case ServerErrorCode::InstallationFailed:
			return "An error occured while installing the app.";

		case ServerErrorCode::MaximumFreeAppLimitReached:
			return "You have reached the limit of 3 apps per device.\n\nIf you're running iOS 13.5 or later, make sure 'Offload Unused Apps' is disabled in Settings > iTunes & App Stores, then install or delete all offloaded apps to prevent them from erroneously counting towards this limit.";

		case ServerErrorCode::UnsupportediOSVersion:
			return "Your device must be running iOS 12.2 or later to install AltStore.";

		case ServerErrorCode::UnknownRequest:
			return "AltServer does not support this request.";

		case ServerErrorCode::UnknownResponse:
			return "Received an unknown response from AltServer.";

		case ServerErrorCode::InvalidAnisetteData:
			return "Invalid anisette data. Please download the latest versions of iTunes and iCloud directly from Apple, and not from the Microsoft Store.";

		case ServerErrorCode::PluginNotFound:
			return "Could not connect to Mail plug-in. Please make sure the plug-in is installed and Mail is running, then try again.";

		case ServerErrorCode::ProfileNotFound:
			return "Could not find provisioning profile.";

		case ServerErrorCode::AppDeletionFailed:
			return "An error occured while removing the app.";
		}
    }
};

#endif /* ServerError_hpp */

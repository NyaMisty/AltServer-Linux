//
//  InstallError.h
//  AltServer-Windows
//
//  Created by Riley Testut on 8/31/19.
//  Copyright © 2019 Riley Testut. All rights reserved.
//

#ifndef InstallError_h
#define InstallError_h

#include "Error.hpp"

enum class InstallErrorCode
{
	Cancelled,
    NoTeam,
    MissingPrivateKey,
    MissingCertificate,
    MissingInfoPlist,
};

class InstallError: public Error
{
public:
    InstallError(InstallErrorCode code) : Error((int)code)
    {
    }
    
    virtual std::string domain() const
    {
        return "com.rileytestut.AltServer.InstallError";
    }
    
    virtual std::string localizedDescription() const
    {
		switch ((InstallErrorCode)this->code())
		{
		case InstallErrorCode::Cancelled:
			return "The operation was cancelled.";

		case InstallErrorCode::NoTeam:
			return "You are not a member of any developer teams.";

		case InstallErrorCode::MissingPrivateKey:
			return "The developer certificate's private key could not be found.";

		case InstallErrorCode::MissingCertificate:
			return "The developer certificate could not be found.";

		case InstallErrorCode::MissingInfoPlist:
			return "The app's Info.plist could not be found.";
		}
    }
};


#endif /* InstallError_h */

#pragma once

#include <memory>
#include <functional>

#include "Error.hpp"

class AnisetteData;

enum class AnisetteErrorCode
{
	iTunesNotInstalled,
	iCloudNotInstalled,
	MissingApplicationSupportFolder,
	MissingAOSKit,
	MissingObjc,
	MissingFoundation,
	InvalidiTunesInstallation,
};

class AnisetteError : public Error
{
public:
	AnisetteError(AnisetteErrorCode code) : Error((int)code)
	{
	}

	virtual std::string domain() const
	{
		return "com.rileytestut.AltServer.AnisetteError";
	}

	virtual std::string localizedDescription() const
	{
		if (this->_localizedDescription.size() > 0)
		{
			return this->_localizedDescription;
		}

		switch ((AnisetteErrorCode)this->code())
		{
		case AnisetteErrorCode::iTunesNotInstalled: return "iTunes Not Found";
		case AnisetteErrorCode::iCloudNotInstalled: return "iCloud Not Found";
		case AnisetteErrorCode::MissingApplicationSupportFolder: return "Missing 'Application Support' in 'Apple' Folder.";
		case AnisetteErrorCode::MissingAOSKit: return "Missing 'AOSKit.dll' in 'Internet Services' Folder.";
		case AnisetteErrorCode::MissingFoundation: return "Missing 'Foundation.dll' in 'Apple Application Support' Folder.";
		case AnisetteErrorCode::MissingObjc: return "Missing 'objc.dll' in 'Apple Application Support' Folder.";
		case AnisetteErrorCode::InvalidiTunesInstallation: return "Invalid iTunes installation.";
		}

		return "";
	}

	void setLocalizedDescription(std::string localizedDescription)
	{
		_localizedDescription = localizedDescription;
	}

private:
	std::string _localizedDescription;
};

class AnisetteDataManager
{
public:
	static AnisetteDataManager* instance();

	std::shared_ptr<AnisetteData> FetchAnisetteData();
	bool LoadDependencies();

	bool ResetProvisioning();

private:
	AnisetteDataManager();
	~AnisetteDataManager();

	static AnisetteDataManager* _instance;

	bool ReprovisionDevice(std::function<void(void)> provisionCallback);
	bool LoadiCloudDependencies();

	bool loadedDependencies;
};


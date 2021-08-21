//
//  DeviceManager.cpp
//  AltServer-Windows
//
//  Created by Riley Testut on 8/13/19.
//  Copyright © 2019 Riley Testut. All rights reserved.
//

#include "DeviceManager.hpp"

#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/lockdown.h>
#include <libimobiledevice/installation_proxy.h>
#include <libimobiledevice/notification_proxy.h>
#include <libimobiledevice/afc.h>
#include <libimobiledevice/misagent.h>

#include <string.h>

#include <filesystem>

#include <iostream>
#include <fstream>
#include <sstream>
#include <condition_variable>

#include "Archiver.hpp"
#include "ServerError.hpp"
#include "ProvisioningProfile.hpp"
#include "Application.hpp"


#define DEVICE_LISTENING_SOCKET 28151

void DeviceManagerUpdateStatus(plist_t command, plist_t status, void *udid);
void DeviceManagerUpdateAppDeletionStatus(plist_t command, plist_t status, void* udid);
void DeviceDidChangeConnectionStatus(const idevice_event_t* event, void* user_data);

namespace fs = std::filesystem;

extern std::string make_uuid();
extern std::string temporary_directory();
extern std::vector<unsigned char> readFile(const char* filename);

idevice_error_t idevice_new_all(idevice_t *idevice, const char *udid) {
    return idevice_new_with_options(idevice, udid, (idevice_options)(IDEVICE_LOOKUP_USBMUX | IDEVICE_LOOKUP_NETWORK));
}

idevice_error_t idevice_new_ignore_network(idevice_t *idevice, const char *udid) {
    return idevice_new_with_options(idevice, udid, IDEVICE_LOOKUP_USBMUX);
}


/// Returns a version of 'str' where every occurrence of
/// 'find' is substituted by 'replace'.
/// - Inspired by James Kanze.
/// - http://stackoverflow.com/questions/20406744/
std::string replace_all(
	const std::string& str,   // where to work
	const std::string& find,  // substitute 'find'
	const std::string& replace //      by 'replace'
) {
	using namespace std;
	string result;
	size_t find_len = find.size();
	size_t pos, from = 0;
	while (string::npos != (pos = str.find(find, from))) {
		result.append(str, from, pos - from);
		result.append(replace);
		from = pos + find_len;
	}
	result.append(str, from, string::npos);
	return result;
}

DeviceManager* DeviceManager::_instance = nullptr;

DeviceManager* DeviceManager::instance()
{
    if (_instance == 0)
    {
        _instance = new DeviceManager();
    }
    
    return _instance;
}

DeviceManager::DeviceManager()
{
}

void DeviceManager::Start()
{
	idevice_event_subscribe(DeviceDidChangeConnectionStatus, NULL);
}

pplx::task<void> DeviceManager::InstallApp(std::string appFilepath, std::string deviceUDID, std::optional<std::set<std::string>> activeProfiles, std::function<void(double)> progressCompletionHandler)
{
	return pplx::task<void>([=] {
		// Enforce only one installation at a time.
		this->_mutex.lock();

		auto UUID = make_uuid();

		char* uuidString = (char*)malloc(UUID.size() + 1);
		strncpy(uuidString, (const char*)UUID.c_str(), UUID.size());
		uuidString[UUID.size()] = '\0';

		idevice_t device = nullptr;
		lockdownd_client_t client = NULL;
		instproxy_client_t ipc = NULL;
		afc_client_t afc = NULL;
		misagent_client_t mis = NULL;
		lockdownd_service_descriptor_t service = NULL;

		fs::path temporaryDirectory(temporary_directory());
		temporaryDirectory.append(make_uuid());

		fs::create_directory(temporaryDirectory);

		auto installedProfiles = std::make_shared<std::vector<std::shared_ptr<ProvisioningProfile>>>();
		auto cachedProfiles = std::make_shared<std::map<std::string, std::shared_ptr<ProvisioningProfile>>>();

		auto finish = [this, installedProfiles, cachedProfiles, activeProfiles, temporaryDirectory, &uuidString]
		(idevice_t device, lockdownd_client_t client, instproxy_client_t ipc, afc_client_t afc, misagent_client_t mis, lockdownd_service_descriptor_t service)
		{
			auto cleanUp = [=]() {
				instproxy_client_free(ipc);
				afc_client_free(afc);
				lockdownd_client_free(client);
				misagent_client_free(mis);
				idevice_free(device);
				lockdownd_service_descriptor_free(service);

				free(uuidString);

				this->_mutex.unlock();
				// if (fs::exists(temporaryDirectory)) fs::remove_all(temporaryDirectory);
			};

			try
			{
				if (activeProfiles.has_value())
				{
					// Remove installed provisioning profiles if they're not active.
					for (auto& installedProfile : *installedProfiles)
					{
						if (std::count(activeProfiles->begin(), activeProfiles->end(), installedProfile->bundleIdentifier()) == 0)
						{
							this->RemoveProvisioningProfile(installedProfile, mis);
						}
					}
				}

				for (auto& pair : *cachedProfiles)
				{
					BOOL reinstall = true;

					for (auto& installedProfile : *installedProfiles)
					{
						if (installedProfile->bundleIdentifier() == pair.second->bundleIdentifier())
						{
							// Don't reinstall cached profile because it was installed with app.
							reinstall = false;
							break;
						}
					}

					if (reinstall)
					{
						this->InstallProvisioningProfile(pair.second, mis);
					}					
				}				
			}
			catch (std::exception& exception)
			{
				cleanUp();
				throw;
			}

			// Clean up outside scope so if an exception is thrown, we don't
			// catch it ourselves again.
			cleanUp();
		};

		try
		{
			fs::path filepath(appFilepath);

			auto extension = filepath.extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
				return std::tolower(c);
				});

			fs::path appBundlePath;

			if (extension == ".app")
			{
				appBundlePath = filepath;
			}
			else if (extension == ".ipa")
			{
				std::cout << "Unzipping .ipa..." << std::endl;
				appBundlePath = UnzipAppBundle(filepath.string(), temporaryDirectory.string());
			}
			else
			{
				throw SignError(SignErrorCode::InvalidApp);
			}

			std::shared_ptr<Application> application = std::make_shared<Application>(appBundlePath.string());
			if (application == NULL)
			{
				throw SignError(SignErrorCode::InvalidApp);
			}

			if (application->provisioningProfile())
			{
				installedProfiles->push_back(application->provisioningProfile());
			}

			for (auto& appExtension : application->appExtensions())
			{
				if (appExtension->provisioningProfile())
				{
					installedProfiles->push_back(appExtension->provisioningProfile());
				}
			}

			odslog("InstallApp: Finding Device...")
			/* Find Device */
			if (idevice_new_all(&device, deviceUDID.c_str()) != IDEVICE_E_SUCCESS)
			{
				throw ServerError(ServerErrorCode::DeviceNotFound);
			}

			odslog("InstallApp: Connecting Device...")
			/* Connect to Device */
			if (lockdownd_client_new_with_handshake(device, &client, "altserver") != LOCKDOWN_E_SUCCESS)
			{
				throw ServerError(ServerErrorCode::ConnectionFailed);
			}

			odslog("InstallApp: Starting instproxy...")
			/* Connect to Installation Proxy */
			if ((lockdownd_start_service(client, "com.apple.mobile.installation_proxy", &service) != LOCKDOWN_E_SUCCESS) || service == NULL)
			{
				throw ServerError(ServerErrorCode::ConnectionFailed);
			}

			odslog("InstallApp: Connecting instproxy...")
			if (instproxy_client_new(device, service, &ipc) != INSTPROXY_E_SUCCESS)
			{
				throw ServerError(ServerErrorCode::ConnectionFailed);
			}

			if (service)
			{
				lockdownd_service_descriptor_free(service);
				service = NULL;
			}


			odslog("InstallApp: Starting misagent...")
			/* Connect to Misagent */
			// Must connect now, since if we take too long writing files to device, connecting may fail later when managing profiles.
			if (lockdownd_start_service(client, "com.apple.misagent", &service) != LOCKDOWN_E_SUCCESS || service == NULL)
			{
				throw ServerError(ServerErrorCode::ConnectionFailed);
			}

			odslog("InstallApp: Connecting misagent...")
			if (misagent_client_new(device, service, &mis) != MISAGENT_E_SUCCESS)
			{
				throw ServerError(ServerErrorCode::ConnectionFailed);
			}


			odslog("InstallApp: Starting afc...")
			/* Connect to AFC service */
			if ((lockdownd_start_service(client, "com.apple.afc", &service) != LOCKDOWN_E_SUCCESS) || service == NULL)
			{
				throw ServerError(ServerErrorCode::ConnectionFailed);
			}

			odslog("InstallApp: Connecting afc...")
			if (afc_client_new(device, service, &afc) != AFC_E_SUCCESS)
			{
				throw ServerError(ServerErrorCode::ConnectionFailed);
			}

			odslog("InstallApp: Preparing to write files to device...")
			fs::path stagingPath("PublicStaging");

			/* Prepare for installation */
			char** files = NULL;
			if (afc_get_file_info(afc, (const char*)stagingPath.c_str(), &files) != AFC_E_SUCCESS)
			{
				if (afc_make_directory(afc, (const char*)stagingPath.c_str()) != AFC_E_SUCCESS)
				{
					throw ServerError(ServerErrorCode::DeviceWriteFailed);
				}
			}

			if (files)
			{
				int i = 0;

				while (files[i])
				{
					free(files[i]);
					i++;
				}

				free(files);
			}

			std::cout << "Writing to device..." << std::endl;

			plist_t options = instproxy_client_options_new();
			instproxy_client_options_add(options, "PackageType", "Developer", NULL);

			fs::path destinationPath = stagingPath.append(appBundlePath.filename().string());

			int numberOfFiles = 0;
			for (auto& item : fs::recursive_directory_iterator(appBundlePath))
			{
				if (item.is_regular_file())
				{
					numberOfFiles++;
				}				
			}

			int writtenFiles = 0;

			try
			{
				this->WriteDirectory(afc, appBundlePath.string(), destinationPath.string(), [&writtenFiles, numberOfFiles, progressCompletionHandler](std::string filepath) {
					writtenFiles++;

					double progress = (double)writtenFiles / (double)numberOfFiles;
					double weightedProgress = progress * 0.75;
					progressCompletionHandler(weightedProgress);
				});
			}
			catch (ServerError& e)
			{
				if (application->bundleIdentifier().find("science.xnu.undecimus") != std::string::npos)
				{
					auto userInfo = e.userInfo();
					userInfo["NSLocalizedRecoverySuggestion"] = "Make sure Windows real-time protection is disabled on your computer then try again.";

					throw ServerError((ServerErrorCode)e.code(), userInfo);
				}	
				else
				{
					throw;
				}				
			}
			catch (std::exception& exception)
			{
				if (application->bundleIdentifier().find("science.xnu.undecimus") != std::string::npos)
				{
					std::map<std::string, std::string> userInfo = {
						{ "NSLocalizedDescription", exception.what() },
						{ "NSLocalizedRecoverySuggestion", "Make sure Windows real-time protection is disabled on your computer then try again." }
					};

					if (std::string(exception.what()) == std::string("vector<T> too long"))
					{
						userInfo["NSLocalizedFailureReason"] = "Windows Defender Blocked Installation";
					}
					else
					{
						userInfo["NSLocalizedFailureReason"] = exception.what();
					}

					throw ServerError(ServerErrorCode::Unknown, userInfo);
				}
				else
				{
					throw;
				}
			}

			std::cout << "Finished writing to device." << std::endl;


			if (service)
			{
				lockdownd_service_descriptor_free(service);
				service = NULL;
			}

			/* Provisioning Profiles */			
			bool shouldManageProfiles = (activeProfiles.has_value() || (application->provisioningProfile() != NULL && application->provisioningProfile()->isFreeProvisioningProfile()));
			if (shouldManageProfiles)
			{				
				// Free developer account was used to sign this app, so we need to remove all
				// provisioning profiles in order to remain under sideloaded app limit.

				auto removedProfiles = this->RemoveAllFreeProvisioningProfilesExcludingBundleIdentifiers({}, mis);
				for (auto& pair : removedProfiles)
				{
					if (activeProfiles.has_value())
					{
						if (activeProfiles->count(pair.first) > 0)
						{
							// Only cache active profiles to reinstall afterwards.
							(*cachedProfiles)[pair.first] = pair.second;
						}
					}
					else
					{
						// Cache all profiles to reinstall afterwards if we didn't provide activeProfiles.
						(*cachedProfiles)[pair.first] = pair.second;
					}
				}				
			}

			lockdownd_client_free(client);
			client = NULL;

			std::mutex waitingMutex;
			std::condition_variable cv;

			std::optional<ServerError> serverError = std::nullopt;
			std::optional<LocalizedError> localizedError = std::nullopt;

			bool didBeginInstalling = false;
			bool didFinishInstalling = false;

			this->_installationProgressHandlers[UUID] = [device, client, ipc, afc, mis, service, finish, progressCompletionHandler, 
				&waitingMutex, &cv, &didBeginInstalling, &didFinishInstalling, &serverError, &localizedError](double progress, int resultCode, char *name, char *description) {
				double weightedProgress = progress * 0.25;
				double adjustedProgress = weightedProgress + 0.75;

				if (progress == 0 && didBeginInstalling)
				{
					if (resultCode != 0 || name != NULL)
					{
						if (resultCode == -402620383)
						{
							std::map<std::string, std::string> userInfo = {
								{ "NSLocalizedRecoverySuggestion", "Make sure 'Offload Unused Apps' is disabled in Settings > iTunes & App Stores, then install or delete all offloaded apps." }
							};
							serverError = std::make_optional<ServerError>(ServerErrorCode::MaximumFreeAppLimitReached, userInfo);
						}
						else
						{
							std::string errorName(name);

							if (errorName == "DeviceOSVersionTooLow")
							{
								serverError = std::make_optional<ServerError>(ServerErrorCode::UnsupportediOSVersion);
							}
							else
							{
								localizedError = std::make_optional<LocalizedError>(resultCode, description);
							}
						}
					}

					std::lock_guard<std::mutex> lock(waitingMutex);
					didFinishInstalling = true;
					cv.notify_all();
				}
				else
				{
					progressCompletionHandler(adjustedProgress);
				}

				didBeginInstalling = true;
			};

			auto narrowDestinationPath = destinationPath.string();
			std::replace(narrowDestinationPath.begin(), narrowDestinationPath.end(), '\\', '/');

			instproxy_install(ipc, narrowDestinationPath.c_str(), options, DeviceManagerUpdateStatus, uuidString);
			instproxy_client_options_free(options);

			// Wait until we're finished installing;
			std::unique_lock<std::mutex> lock(waitingMutex);
			cv.wait(lock, [&didFinishInstalling] { return didFinishInstalling; });

			lock.unlock();

			if (serverError.has_value())
			{
				throw serverError.value();
			}

			if (localizedError.has_value())
			{
				throw localizedError.value();
			}
		}
		catch (std::exception& exception)
		{
			try
			{
				// MUST finish so we restore provisioning profiles.
				finish(device, client, ipc, afc, mis, service);
			}
			catch (std::exception& e)
			{
				// Ignore since we already caught an exception during installation.
			}

			throw;
		}

		// Call finish outside try-block so if an exception is thrown, we don't
		// catch it ourselves and "finish" again.
		finish(device, client, ipc, afc, mis, service);
	});
}

void DeviceManager::WriteDirectory(afc_client_t client, std::string directoryPath, std::string destinationPath, std::function<void(std::string)> wroteFileCallback)
{
	std::replace(destinationPath.begin(), destinationPath.end(), '\\', '/');

    afc_make_directory(client, destinationPath.c_str());
    
    for (auto& file : fs::directory_iterator(directoryPath))
    {
        auto filepath = file.path();
        
        if (fs::is_directory(filepath))
        {
            auto destinationDirectoryPath = fs::path(destinationPath).append(filepath.filename().string());
            this->WriteDirectory(client, filepath.string(), destinationDirectoryPath.string(), wroteFileCallback);
        }
        else
        {
            auto destinationFilepath = fs::path(destinationPath).append(filepath.filename().string());
            this->WriteFile(client, filepath.string(), destinationFilepath.string(), wroteFileCallback);
        }
    }
}

void DeviceManager::WriteFile(afc_client_t client, std::string filepath, std::string destinationPath, std::function<void(std::string)> wroteFileCallback)
{
	std::replace(destinationPath.begin(), destinationPath.end(), '\\', '/');
	destinationPath = replace_all(destinationPath, "__colon__", ":");

	odslog("Writing File: " << filepath.c_str() << " to: " << destinationPath.c_str());
    
    auto data = readFile(filepath.c_str());
    
    uint64_t af = 0;
    if ((afc_file_open(client, destinationPath.c_str(), AFC_FOPEN_WRONLY, &af) != AFC_E_SUCCESS) || af == 0)
    {
        throw ServerError(ServerErrorCode::DeviceWriteFailed);
    }
    
    uint32_t bytesWritten = 0;
    
    while (bytesWritten < data.size())
    {
        uint32_t count = 0;
        
        if (afc_file_write(client, af, (const char *)data.data() + bytesWritten, (uint32_t)data.size() - bytesWritten, &count) != AFC_E_SUCCESS)
        {
            throw ServerError(ServerErrorCode::DeviceWriteFailed);
        }
        
        bytesWritten += count;
    }
    
    if (bytesWritten != data.size())
    {
        throw ServerError(ServerErrorCode::DeviceWriteFailed);
    }
    
    afc_file_close(client, af);

	wroteFileCallback(filepath);
}

pplx::task<void> DeviceManager::RemoveApp(std::string bundleIdentifier, std::string deviceUDID)
{
	return pplx::task<void>([=] {
		idevice_t device = NULL;
		lockdownd_client_t client = NULL;
		instproxy_client_t ipc = NULL;
		lockdownd_service_descriptor_t service = NULL;

		auto cleanUp = [&]() {
			if (service) {
				lockdownd_service_descriptor_free(service);
			}

			if (ipc) {
				instproxy_client_free(ipc);
			}

			if (client) {
				lockdownd_client_free(client);
			}

			if (device) {
				idevice_free(device);
			}
		};

		try 
		{
			/* Find Device */
			if (idevice_new_all(&device, deviceUDID.c_str()) != IDEVICE_E_SUCCESS)
			{
				throw ServerError(ServerErrorCode::DeviceNotFound);
			}

			/* Connect to Device */
			if (lockdownd_client_new_with_handshake(device, &client, "altserver") != LOCKDOWN_E_SUCCESS)
			{
				throw ServerError(ServerErrorCode::ConnectionFailed);
			}

			/* Connect to Installation Proxy */
			if ((lockdownd_start_service(client, "com.apple.mobile.installation_proxy", &service) != LOCKDOWN_E_SUCCESS) || service == NULL)
			{
				throw ServerError(ServerErrorCode::ConnectionFailed);
			}

			if (instproxy_client_new(device, service, &ipc) != INSTPROXY_E_SUCCESS)
			{
				throw ServerError(ServerErrorCode::ConnectionFailed);
			}

			if (service)
			{
				lockdownd_service_descriptor_free(service);
				service = NULL;
			}

			auto UUID = make_uuid();

			char* uuidString = (char*)malloc(UUID.size() + 1);
			strncpy(uuidString, (const char*)UUID.c_str(), UUID.size());
			uuidString[UUID.size()] = '\0';

			std::mutex waitingMutex;
			std::condition_variable cv;

			std::optional<ServerError> serverError = std::nullopt;

			bool didFinishInstalling = false;

			this->_deletionCompletionHandlers[UUID] = [this, &waitingMutex, &cv, &didFinishInstalling, &serverError, &uuidString]
			(bool success, int errorCode, char* errorName, char* errorDescription) {
				if (!success)
				{
					std::map<std::string, std::string> userInfo = { 
						{ "NSLocalizedFailure", ServerError(ServerErrorCode::AppDeletionFailed).localizedDescription() }, 
						{ "NSLocalizedFailureReason", errorDescription } 
					};
					serverError = std::make_optional<ServerError>(ServerErrorCode::AppDeletionFailed, userInfo);
				}

				std::lock_guard<std::mutex> lock(waitingMutex);
				didFinishInstalling = true;
				cv.notify_all();

				free(uuidString);
			};

			instproxy_uninstall(ipc, bundleIdentifier.c_str(), NULL, DeviceManagerUpdateAppDeletionStatus, uuidString);

			// Wait until we're finished installing;
			std::unique_lock<std::mutex> lock(waitingMutex);
			cv.wait(lock, [&didFinishInstalling] { return didFinishInstalling; });

			lock.unlock();

			if (serverError.has_value())
			{
				throw serverError.value();
			}

			cleanUp();
		}
		catch (std::exception& exception) {
			cleanUp();
			throw;
		}
	});
}

pplx::task<std::shared_ptr<WiredConnection>> DeviceManager::StartWiredConnection(std::shared_ptr<Device> altDevice)
{
	return pplx::create_task([=]() -> std::shared_ptr<WiredConnection> {
		idevice_t device = NULL;
		idevice_connection_t connection = NULL;

		/* Find Device */
		if (idevice_new_ignore_network(&device, altDevice->identifier().c_str()) != IDEVICE_E_SUCCESS)
		{
			throw ServerError(ServerErrorCode::DeviceNotFound);
		}

		/* Connect to Listening Socket */
		if (idevice_connect(device, DEVICE_LISTENING_SOCKET, &connection) != IDEVICE_E_SUCCESS)
		{
			idevice_free(device);
			throw ServerError(ServerErrorCode::ConnectionFailed);
		}

		idevice_free(device);

		auto wiredConnection = std::make_shared<WiredConnection>(altDevice, connection);
		return wiredConnection;
	});
}

pplx::task<void> DeviceManager::InstallProvisioningProfiles(std::vector<std::shared_ptr<ProvisioningProfile>> provisioningProfiles, std::string deviceUDID, std::optional<std::set<std::string>> activeProfiles)
{
	return pplx::task<void>([=] {
		// Enforce only one installation at a time.
		this->_mutex.lock();

		idevice_t device = NULL;
		lockdownd_client_t client = NULL;
		afc_client_t afc = NULL;
		misagent_client_t mis = NULL;
		lockdownd_service_descriptor_t service = NULL;

		auto cleanUp = [&]() {
			if (service) {
				lockdownd_service_descriptor_free(service);
			}

			if (mis) {
				misagent_client_free(mis);
			}

			if (afc) {
				afc_client_free(afc);
			}

			if (client) {
				lockdownd_client_free(client);
			}

			if (device) {
				idevice_free(device);
			}

			this->_mutex.unlock();
		};

		try
		{
			/* Find Device */
			if (idevice_new_all(&device, deviceUDID.c_str()) != IDEVICE_E_SUCCESS)
			{
				throw ServerError(ServerErrorCode::DeviceNotFound);
			}

			/* Connect to Device */
			if (lockdownd_client_new_with_handshake(device, &client, "altserver") != LOCKDOWN_E_SUCCESS)
			{
				throw ServerError(ServerErrorCode::ConnectionFailed);
			}

			/* Connect to Misagent */
			if (lockdownd_start_service(client, "com.apple.misagent", &service) != LOCKDOWN_E_SUCCESS || service == NULL)
			{
				throw ServerError(ServerErrorCode::ConnectionFailed);
			}

			if (misagent_client_new(device, service, &mis) != MISAGENT_E_SUCCESS)
			{
				throw ServerError(ServerErrorCode::ConnectionFailed);
			}

			if (activeProfiles.has_value())
			{
				// Remove all non-active free provisioning profiles.

				auto excludedBundleIdentifiers = activeProfiles.value();
				for (auto& profile : provisioningProfiles)
				{
					// Ensure we DO remove old versions of profiles we're about to install, even if they are active.
					excludedBundleIdentifiers.erase(profile->bundleIdentifier());
				}

				this->RemoveAllFreeProvisioningProfilesExcludingBundleIdentifiers(excludedBundleIdentifiers, mis);
			}
			else
			{
				// Remove only older versions of provisioning profiles we're about to install.

				std::set<std::string> bundleIdentifiers;
				for (auto& profile : provisioningProfiles)
				{
					bundleIdentifiers.insert(profile->bundleIdentifier());
				}

				this->RemoveProvisioningProfiles(bundleIdentifiers, mis);
			}

			for (auto& provisioningProfile : provisioningProfiles)
			{
				this->InstallProvisioningProfile(provisioningProfile, mis);
			}

			cleanUp();
		}
		catch (std::exception &exception)
		{
			cleanUp();
			throw;
		}
	});
}

pplx::task<void> DeviceManager::RemoveProvisioningProfiles(std::set<std::string> bundleIdentifiers, std::string deviceUDID)
{
	return pplx::task<void>([=] {
		// Enforce only one removal at a time.
		this->_mutex.lock();

		idevice_t device = NULL;
		lockdownd_client_t client = NULL;
		afc_client_t afc = NULL;
		misagent_client_t mis = NULL;
		lockdownd_service_descriptor_t service = NULL;

		auto cleanUp = [&]() {
			if (service) {
				lockdownd_service_descriptor_free(service);
			}

			if (mis) {
				misagent_client_free(mis);
			}

			if (afc) {
				afc_client_free(afc);
			}

			if (client) {
				lockdownd_client_free(client);
			}

			if (device) {
				idevice_free(device);
			}

			this->_mutex.unlock();
		};

		try
		{
			/* Find Device */
			if (idevice_new_all(&device, deviceUDID.c_str()) != IDEVICE_E_SUCCESS)
			{
				throw ServerError(ServerErrorCode::DeviceNotFound);
			}

			/* Connect to Device */
			if (lockdownd_client_new_with_handshake(device, &client, "altserver") != LOCKDOWN_E_SUCCESS)
			{
				throw ServerError(ServerErrorCode::ConnectionFailed);
			}

			/* Connect to Misagent */
			if (lockdownd_start_service(client, "com.apple.misagent", &service) != LOCKDOWN_E_SUCCESS || service == NULL)
			{
				throw ServerError(ServerErrorCode::ConnectionFailed);
			}

			if (misagent_client_new(device, service, &mis) != MISAGENT_E_SUCCESS)
			{
				throw ServerError(ServerErrorCode::ConnectionFailed);
			}

			this->RemoveProvisioningProfiles(bundleIdentifiers, mis);

			cleanUp();
		}
		catch (std::exception& exception)
		{
			cleanUp();
			throw;
		}
	});
}

std::map<std::string, std::shared_ptr<ProvisioningProfile>> DeviceManager::RemoveProvisioningProfiles(std::set<std::string> bundleIdentifiers, misagent_client_t mis)
{
	return this->RemoveAllProvisioningProfiles(bundleIdentifiers, std::nullopt, false, mis);
}

std::map<std::string, std::shared_ptr<ProvisioningProfile>> DeviceManager::RemoveAllFreeProvisioningProfilesExcludingBundleIdentifiers(std::set<std::string> excludedBundleIdentifiers, misagent_client_t mis)
{
	return this->RemoveAllProvisioningProfiles(std::nullopt, excludedBundleIdentifiers, true, mis);
}

std::map<std::string, std::shared_ptr<ProvisioningProfile>> DeviceManager::RemoveAllProvisioningProfiles(std::optional<std::set<std::string>> includedBundleIdentifiers, std::optional<std::set<std::string>> excludedBundleIdentifiers, bool limitedToFreeProfiles, misagent_client_t mis)
{
	std::map<std::string, std::shared_ptr<ProvisioningProfile>> ignoredProfiles;
	std::map<std::string, std::shared_ptr<ProvisioningProfile>> removedProfiles;

	auto provisioningProfiles = this->CopyProvisioningProfiles(mis);

	for (auto& provisioningProfile : provisioningProfiles)
	{
		if (limitedToFreeProfiles && !provisioningProfile->isFreeProvisioningProfile())
		{
			continue;
		}

		if (includedBundleIdentifiers.has_value() && includedBundleIdentifiers->count(provisioningProfile->bundleIdentifier()) == 0)
		{
			continue;
		}

		if (excludedBundleIdentifiers.has_value() && excludedBundleIdentifiers->count(provisioningProfile->bundleIdentifier()) > 0)
		{
			// This provisioning profile has an excluded bundle identifier.
			// Ignore it, unless we've already ignored one with the same bundle identifier,
			// in which case remove whichever profile is the oldest.

			auto previousProfile = ignoredProfiles[provisioningProfile->bundleIdentifier()];
			if (previousProfile != NULL)
			{
				auto expirationDateA = provisioningProfile->expirationDate();
				auto expirationDateB = previousProfile->expirationDate();

				// We've already ignored a profile with this bundle identifier,
				// so make sure we only ignore the newest one and remove the oldest one.
				BOOL newerThanPreviousProfile = (timercmp(&expirationDateA, &expirationDateB, >) != 0);
				auto oldestProfile = newerThanPreviousProfile ? previousProfile : provisioningProfile;
				auto newestProfile = newerThanPreviousProfile ? provisioningProfile : previousProfile;

				ignoredProfiles[provisioningProfile->bundleIdentifier()] = newestProfile;

				// Don't cache this profile or else it will be reinstalled, so just remove it without caching.
				this->RemoveProvisioningProfile(oldestProfile, mis);
			}
			else
			{
				ignoredProfiles[provisioningProfile->bundleIdentifier()] = provisioningProfile;
			}

			continue;
		}

		auto preferredProfile = removedProfiles[provisioningProfile->bundleIdentifier()];
		if (preferredProfile != nullptr)
		{
			auto expirationDateA = provisioningProfile->expirationDate();
			auto expirationDateB = preferredProfile->expirationDate();

			if (timercmp(&expirationDateA, &expirationDateB, > ) != 0)
			{
				// provisioningProfile exires later than preferredProfile, so use provisioningProfile instead.
				removedProfiles[provisioningProfile->bundleIdentifier()] = provisioningProfile;
			}
		}
		else
		{
			removedProfiles[provisioningProfile->bundleIdentifier()] = provisioningProfile;
		}

		this->RemoveProvisioningProfile(provisioningProfile, mis);
	}

	return removedProfiles;
}

void DeviceManager::InstallProvisioningProfile(std::shared_ptr<ProvisioningProfile> profile, misagent_client_t mis)
{
	plist_t pdata = plist_new_data((const char*)profile->data().data(), profile->data().size());

	misagent_error_t result = misagent_install(mis, pdata);
	plist_free(pdata);

	if (result == MISAGENT_E_SUCCESS)
	{
		odslog("Installed profile: " << (profile->bundleIdentifier()) << " (" << (profile->uuid()) << ")");
	}
	else
	{
		int statusCode = misagent_get_status_code(mis);
		odslog("Failed to install provisioning profile: " << (profile->bundleIdentifier()) << " (" << (profile->uuid()) << "). Error code: " << statusCode);

		switch (statusCode)
		{
		case -402620383:
		{
			std::map<std::string, std::string> userInfo = {
				{ "NSLocalizedRecoverySuggestion", "Make sure 'Offload Unused Apps' is disabled in Settings > iTunes & App Stores, then install or delete all offloaded apps." }
			};
			throw ServerError(ServerErrorCode::MaximumFreeAppLimitReached, userInfo);
		}

		default:
		{
			std::ostringstream oss;
			oss << "Could not install profile '" << profile->bundleIdentifier() << "'";

			std::string localizedFailure = oss.str();

			std::map<std::string, std::string> userInfo = {
					{ LocalizedFailureErrorKey, localizedFailure },
					{ ProvisioningProfileBundleIDErrorKey, profile->bundleIdentifier() },
					{ UnderlyingErrorCodeErrorKey, std::to_string(statusCode) }
			};

			throw ServerError(ServerErrorCode::UnderlyingError, userInfo);
		}
		}
	}
}

void DeviceManager::RemoveProvisioningProfile(std::shared_ptr<ProvisioningProfile> profile, misagent_client_t mis)
{
	std::string uuid = profile->uuid();
	std::transform(uuid.begin(), uuid.end(), uuid.begin(), [](unsigned char c) { return std::tolower(c); });

	misagent_error_t result = misagent_remove(mis, uuid.c_str());
	if (result == MISAGENT_E_SUCCESS)
	{
		odslog("Removed profile: " << (profile->bundleIdentifier()) << " (" << (profile->uuid()) << ")");
	}
	else
	{
		int statusCode = misagent_get_status_code(mis);
		odslog("Failed to remove provisioning profile: " << (profile->bundleIdentifier()) << " (" << (profile->uuid()) << "). Error code: " << statusCode);

		switch (statusCode)
		{
		case -402620405:
		{
			std::map<std::string, std::string> userInfo = {
				{ ProvisioningProfileBundleIDErrorKey, profile->bundleIdentifier() },
			};

			throw ServerError(ServerErrorCode::ProfileNotFound, userInfo);
		}

		default:
		{
			std::ostringstream oss;
			oss << "Could not remove profile '" << profile->bundleIdentifier() << "'";

			std::string localizedFailure = oss.str();

			std::map<std::string, std::string> userInfo = {
					{ LocalizedFailureErrorKey, localizedFailure },
					{ ProvisioningProfileBundleIDErrorKey, profile->bundleIdentifier() },
					{ UnderlyingErrorCodeErrorKey, std::to_string(statusCode) }
			};

			throw ServerError(ServerErrorCode::UnderlyingError, userInfo);
		}
		}
	}
}

std::vector<std::shared_ptr<ProvisioningProfile>> DeviceManager::CopyProvisioningProfiles(misagent_client_t mis)
{
	std::vector<std::shared_ptr<ProvisioningProfile>> provisioningProfiles;

	plist_t profiles = NULL;

	if (misagent_copy_all(mis, &profiles) != MISAGENT_E_SUCCESS)
	{
		int statusCode = misagent_get_status_code(mis);

		std::string localizedFailure = "Could not copy provisioning profiles.";

		std::map<std::string, std::string> userInfo = {
				{ LocalizedFailureErrorKey, localizedFailure },
				{ UnderlyingErrorCodeErrorKey, std::to_string(statusCode) }
		};

		throw ServerError(ServerErrorCode::UnderlyingError, userInfo);
	}

	uint32_t profileCount = plist_array_get_size(profiles);
	for (int i = 0; i < profileCount; i++)
	{
		plist_t profile = plist_array_get_item(profiles, i);
		if (plist_get_node_type(profile) != PLIST_DATA)
		{
			continue;
		}

		char* bytes = NULL;
		uint64_t length = 0;

		plist_get_data_val(profile, &bytes, &length);
		if (bytes == NULL)
		{
			continue;
		}

		std::vector<unsigned char> data;
		data.reserve(length);

		for (int i = 0; i < length; i++)
		{
			data.push_back(bytes[i]);
		}

		auto provisioningProfile = std::make_shared<ProvisioningProfile>(data);
		provisioningProfiles.push_back(provisioningProfile);
	}

	plist_free(profiles);

	return provisioningProfiles;
}

pplx::task<std::shared_ptr<NotificationConnection>> DeviceManager::StartNotificationConnection(std::shared_ptr<Device> altDevice)
{
	return pplx::create_task([=]() -> std::shared_ptr<NotificationConnection> {
		idevice_t device = NULL;
		lockdownd_client_t lockdownClient = NULL;
		lockdownd_service_descriptor_t service = NULL;
		np_client_t client = NULL;

		/* Find Device */
		if (idevice_new_ignore_network(&device, altDevice->identifier().c_str()) != IDEVICE_E_SUCCESS)
		{
			throw ServerError(ServerErrorCode::DeviceNotFound);
		}

		/* Connect to Device */
		if (lockdownd_client_new_with_handshake(device, &lockdownClient, "altserver") != LOCKDOWN_E_SUCCESS)
		{
			idevice_free(device);
			throw ServerError(ServerErrorCode::ConnectionFailed);
		}

		/* Connect to Notification Proxy */
		if ((lockdownd_start_service(lockdownClient, "com.apple.mobile.notification_proxy", &service) != LOCKDOWN_E_SUCCESS) || service == NULL)
		{
			lockdownd_client_free(lockdownClient);
			idevice_free(device);

			throw ServerError(ServerErrorCode::ConnectionFailed);
		}

		/* Connect to Client */
		if (np_client_new(device, service, &client) != NP_E_SUCCESS)
		{
			lockdownd_service_descriptor_free(service);
			lockdownd_client_free(lockdownClient);
			idevice_free(device);

			throw ServerError(ServerErrorCode::ConnectionFailed);
		}

		lockdownd_service_descriptor_free(service);
		lockdownd_client_free(lockdownClient);
		idevice_free(device);

		auto notificationConnection = std::make_shared<NotificationConnection>(altDevice, client);
		return notificationConnection;
	});
}

std::vector<std::shared_ptr<Device>> DeviceManager::connectedDevices() const
{
    auto devices = this->availableDevices(false);
    return devices;
}

std::vector<std::shared_ptr<Device>> DeviceManager::availableDevices() const
{
    auto devices = this->availableDevices(true);
    return devices;
}

std::vector<std::shared_ptr<Device>> DeviceManager::availableDevices(bool includeNetworkDevices) const
{
    std::vector<std::shared_ptr<Device>> availableDevices;
    
    int count = 0;
    char **udids = NULL;
    if (idevice_get_device_list(&udids, &count) < 0)
    {
        fprintf(stderr, "ERROR: Unable to retrieve device list!\n");
        return availableDevices;
    }
    
    for (int i = 0; i < count; i++)
    {
        char *udid = udids[i];
        
        idevice_t device = NULL;
        
        if (includeNetworkDevices)
        {
            idevice_new_all(&device, udid);
        }
        else
        {
            idevice_new_ignore_network(&device, udid);
        }
        
        if (!device)
        {
            continue;
        }
        
        lockdownd_client_t client = NULL;
        int result = lockdownd_client_new(device, &client, "altserver");
        if (result != LOCKDOWN_E_SUCCESS)
        {
            fprintf(stderr, "ERROR: Connecting to device %s failed! (%d)\n", udid, result);
            
            idevice_free(device);
            
            continue;
        }
        
        char *device_name = NULL;
        if (lockdownd_get_device_name(client, &device_name) != LOCKDOWN_E_SUCCESS || device_name == NULL)
        {
            fprintf(stderr, "ERROR: Could not get device name!\n");
            
            lockdownd_client_free(client);
            idevice_free(device);
            
            continue;
        }

		plist_t device_type_plist = NULL;
		if (lockdownd_get_value(client, NULL, "ProductType", &device_type_plist) != LOCKDOWN_E_SUCCESS)
		{
			odslog("ERROR: Could not get device type for " << device_name);

			lockdownd_client_free(client);
			idevice_free(device);

			continue;
		}

		Device::Type deviceType = Device::Type::iPhone;

		char* device_type_string = NULL;
		plist_get_string_val(device_type_plist, &device_type_string);

		if (std::string(device_type_string).find("iPhone") != std::string::npos ||
			std::string(device_type_string).find("iPod") != std::string::npos)
		{
			deviceType = Device::Type::iPhone;
		}
		else if (std::string(device_type_string).find("iPad") != std::string::npos)
		{
			deviceType = Device::Type::iPad;
		}
		else if (std::string(device_type_string).find("AppleTV") != std::string::npos)
		{
			deviceType = Device::Type::AppleTV;
		}
		else
		{
			odslog("Unknown device type " << device_type_string << " for " << device_name);
			deviceType = Device::Type::None;
		}
        
        lockdownd_client_free(client);
        idevice_free(device);

		bool isDuplicate = false;

		for (auto& device : availableDevices)
		{
			if (device->identifier() == udid)
			{
				// Duplicate.
				isDuplicate = true;
				break;
			}
		}

		if (isDuplicate)
		{
			continue;
		}
        
		auto altDevice = std::make_shared<Device>(device_name, udid, deviceType);
        availableDevices.push_back(altDevice);
        
        if (device_name != NULL)
        {
            free(device_name);
        }
    }
    
    idevice_device_list_free(udids);
    
    return availableDevices;
}

std::function<void(std::shared_ptr<Device>)> DeviceManager::connectedDeviceCallback() const
{
	return _connectedDeviceCallback;
}

void DeviceManager::setConnectedDeviceCallback(std::function<void(std::shared_ptr<Device>)> callback)
{
	_connectedDeviceCallback = callback;
}

std::function<void(std::shared_ptr<Device>)> DeviceManager::disconnectedDeviceCallback() const
{
	return _disconnectedDeviceCallback;
}

void DeviceManager::setDisconnectedDeviceCallback(std::function<void(std::shared_ptr<Device>)> callback)
{
	_disconnectedDeviceCallback = callback;
}

std::map<std::string, std::shared_ptr<Device>>& DeviceManager::cachedDevices()
{
	return _cachedDevices;
}

#pragma mark - Callbacks -

void DeviceManagerUpdateStatus(plist_t command, plist_t status, void *uuid)
{
	if (DeviceManager::instance()->_installationProgressHandlers.count((char*)uuid) == 0)
	{
		return;
	}
    
    int percent = 0;
    instproxy_status_get_percent_complete(status, &percent);

	char* name = NULL;
	char* description = NULL;
	uint64_t code = 0;
	instproxy_status_get_error(status, &name, &description, &code);

	double progress = ((double)percent / 100.0);

	auto progressHandler = DeviceManager::instance()->_installationProgressHandlers[(char*)uuid];
	progressHandler(progress, code, name, description);
}

void DeviceManagerUpdateAppDeletionStatus(plist_t command, plist_t status, void* uuid)
{
	char *statusName = NULL;
	instproxy_status_get_name(status, &statusName);

	char* errorName = NULL;
	char* errorDescription = NULL;
	uint64_t errorCode = 0;
	instproxy_status_get_error(status, &errorName, &errorDescription, &errorCode);

	if (std::string(statusName) == std::string("Complete") || errorCode != 0 || errorName != NULL)
	{
		auto completionHandler = DeviceManager::instance()->_deletionCompletionHandlers[(char*)uuid];
		if (completionHandler != NULL)
		{
			if (errorName == NULL)
			{
				errorName = (char*)"";
			}

			if (errorDescription == NULL)
			{
				errorDescription = (char*)"";
			}

			if (errorCode != 0 || std::string(errorName) != std::string())
			{
				odslog("Error removing app. " << errorCode << " (" << errorName << "). " << errorDescription);
				completionHandler(false, errorCode, errorName, errorDescription);
			}
			else
			{
				odslog("Finished removing app!");
				completionHandler(true, 0, errorName, errorDescription);
			}

			DeviceManager::instance()->_deletionCompletionHandlers.erase((char*)uuid);
		}
	}
}

void DeviceDidChangeConnectionStatus(const idevice_event_t* event, void* user_data)
{
	switch (event->event)
	{
	case IDEVICE_DEVICE_ADD:
	{
		auto devices = DeviceManager::instance()->connectedDevices();
		std::shared_ptr<Device> device = NULL;

		for (auto& d : devices)
		{
			if (d->identifier() == event->udid)
			{
				device = d;
				break;
			}
		}

		if (device == NULL)
		{
			return;
		}

		if (DeviceManager::instance()->cachedDevices().count(device->identifier()) > 0)
		{
			return;
		}

		odslog("Detected device:" << device->name().c_str());

		DeviceManager::instance()->cachedDevices()[device->identifier()] = device;

		if (DeviceManager::instance()->connectedDeviceCallback() != NULL)
		{
			DeviceManager::instance()->connectedDeviceCallback()(device);
		}
		
		break;
	}
	case IDEVICE_DEVICE_REMOVE:
	{
		auto devices = DeviceManager::instance()->cachedDevices();
		std::shared_ptr<Device> device = DeviceManager::instance()->cachedDevices()[event->udid];

		if (device == NULL)
		{
			return;
		}

		DeviceManager::instance()->cachedDevices().erase(device->identifier());

		if (DeviceManager::instance()->disconnectedDeviceCallback() != NULL)
		{
			DeviceManager::instance()->disconnectedDeviceCallback()(device);
		}

		break;
	}
    default:
        break;
	}
}

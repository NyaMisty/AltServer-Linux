//
//  DeviceManager.hpp
//  AltServer-Windows
//
//  Created by Riley Testut on 8/13/19.
//  Copyright © 2019 Riley Testut. All rights reserved.
//

#ifndef DeviceManager_hpp
#define DeviceManager_hpp

#include "common.h"
#include "Device.hpp"
#include "ProvisioningProfile.hpp"

#include <vector>
#include <map>
#include <set>
#include <mutex>

#include <pplx/pplxtasks.h>
#include <libimobiledevice/afc.h>
#include <libimobiledevice/misagent.h>

#include "WiredConnection.h"
#include "NotificationConnection.h"

class DeviceManager
{
public:
    static DeviceManager *instance();
    
    DeviceManager();
    
    std::vector<std::shared_ptr<Device>> connectedDevices() const;
    std::vector<std::shared_ptr<Device>> availableDevices() const;

	void Start();

	pplx::task<void> InstallApp(std::string filepath, std::string deviceUDID, std::optional<std::set<std::string>> activeProvisioningProfiles, std::function<void(double)> progressCompletionHandler);
	pplx::task<void> RemoveApp(std::string bundleIdentifier, std::string deviceUDID);

	pplx::task<std::shared_ptr<WiredConnection>> StartWiredConnection(std::shared_ptr<Device> device);
	pplx::task<std::shared_ptr<NotificationConnection>> StartNotificationConnection(std::shared_ptr<Device> device);

	pplx::task<void> InstallProvisioningProfiles(std::vector<std::shared_ptr<ProvisioningProfile>> profiles, std::string deviceUDID, std::optional<std::set<std::string>> activeProfiles);
	pplx::task<void> RemoveProvisioningProfiles(std::set<std::string> bundleIdentifiers, std::string deviceUDID);

	std::map<std::string, std::shared_ptr<ProvisioningProfile>> RemoveProvisioningProfiles(std::set<std::string> bundleIdentifiers, misagent_client_t misagent);
	std::map<std::string, std::shared_ptr<ProvisioningProfile>> RemoveAllFreeProvisioningProfilesExcludingBundleIdentifiers(std::set<std::string> excludedBundleIdentifiers, misagent_client_t misagent);
	std::map<std::string, std::shared_ptr<ProvisioningProfile>> RemoveAllProvisioningProfiles(std::optional<std::set<std::string>> includedBundleIdentifiers, std::optional<std::set<std::string>> excludedBundleIdentifiers, bool limitedToFreeProfiles, misagent_client_t misagent);

	std::function<void(std::shared_ptr<Device>)> connectedDeviceCallback() const;
	void setConnectedDeviceCallback(std::function<void(std::shared_ptr<Device>)> callback);

	std::function<void(std::shared_ptr<Device>)> disconnectedDeviceCallback() const;
	void setDisconnectedDeviceCallback(std::function<void(std::shared_ptr<Device>)> callback);
    
private:
    ~DeviceManager();
    
    static DeviceManager *_instance;

	std::mutex _mutex;

	std::map<std::string, std::function<void(double, int, char *, char *)>> _installationProgressHandlers;
	std::map<std::string, std::function<void(bool, int, char*, char*)>> _deletionCompletionHandlers;

	std::function<void(std::shared_ptr<Device>)> _connectedDeviceCallback;
	std::function<void(std::shared_ptr<Device>)> _disconnectedDeviceCallback;

	std::map<std::string, std::shared_ptr<Device>> _cachedDevices;
	std::map<std::string, std::shared_ptr<Device>>& cachedDevices();
    
    std::vector<std::shared_ptr<Device>> availableDevices(bool includeNetworkDevices) const;
    
    void WriteDirectory(afc_client_t client, std::string directoryPath, std::string destinationPath, std::function<void(std::string)> wroteFileCallback);
    void WriteFile(afc_client_t client, std::string filepath, std::string destinationPath, std::function<void(std::string)> wroteFileCallback);

	void InstallProvisioningProfile(std::shared_ptr<ProvisioningProfile> provisioningProfile, misagent_client_t mis);
	void RemoveProvisioningProfile(std::shared_ptr<ProvisioningProfile> provisioningProfile, misagent_client_t mis);
	std::vector<std::shared_ptr<ProvisioningProfile>> CopyProvisioningProfiles(misagent_client_t mis);

	friend void DeviceManagerUpdateStatus(plist_t command, plist_t status, void* uuid);
	friend void DeviceManagerUpdateAppDeletionStatus(plist_t command, plist_t status, void* udid);
	friend void DeviceDidChangeConnectionStatus(const idevice_event_t* event, void* user_data);
};

#endif /* DeviceManager_hpp */

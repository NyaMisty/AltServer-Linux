#pragma once

#include "Device.hpp"

#include <pplx/pplxtasks.h>
#include <libimobiledevice/libimobiledevice.h>
#include <libimobiledevice/notification_proxy.h>

#include <memory>

class NotificationConnection
{
public:
	NotificationConnection(std::shared_ptr<Device> device, np_client_t client);
	~NotificationConnection();

	void Disconnect();

	void StartListening(std::vector<std::string> notifications);
	void SendNotification(std::string notification);

	std::function<void(std::string)> receivedNotificationHandler() const;
	void setReceivedNotificationHandler(std::function<void(std::string)> handler);

	std::shared_ptr<Device> device() const;

	bool active() const;

private:
	std::shared_ptr<Device> _device;
	np_client_t _client;
	std::function<void(std::string)> _receivedNotificationHandler;

	bool _active;

	np_client_t client() const;
};


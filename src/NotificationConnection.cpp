#include "common.h"
#include "NotificationConnection.h"
#include "ServerError.hpp"

#include <cpprest/json.h>

void ALTDeviceReceivedNotification(const char* notification, void* user_data)
{
	NotificationConnection* connection = (NotificationConnection*)user_data;

	if (std::string(notification).size() > 0 && connection->active())
	{
		connection->receivedNotificationHandler()(notification);
	}
}

NotificationConnection::NotificationConnection(std::shared_ptr<Device> device, np_client_t client) : _device(device), _client(client)
{
	this->setReceivedNotificationHandler([](std::string notification) {
		odslog("Received Notification: " << notification);
	});
}

NotificationConnection::~NotificationConnection()
{
	this->Disconnect();
}

void NotificationConnection::Disconnect()
{
	if (_client == NULL)
	{
		return;
	}

	np_client_free(_client);
	_client = NULL;

	// Prevent us from receiving callbacks after deallocation.
	_active = false;
}

void NotificationConnection::StartListening(std::vector<std::string> notifications)
{
	const char** notificationNames = (const char**)malloc((notifications.size() + 1) * sizeof(char *));
	for (int i = 0; i < notifications.size(); i++)
	{
		const char* cName = notifications[i].c_str();
		notificationNames[i] = cName;
	}
	notificationNames[notifications.size()] = NULL; // Must have terminating NULL entry.

	np_error_t result = np_observe_notifications(this->client(), notificationNames);
	if (result != NP_E_SUCCESS)
	{
		throw ServerError(ServerErrorCode::LostConnection);
	}

	result = np_set_notify_callback(this->client(), ALTDeviceReceivedNotification, this);
	if (result != NP_E_SUCCESS)
	{
		throw ServerError(ServerErrorCode::LostConnection);
	}

	_active = true;

	free(notificationNames);
}

void NotificationConnection::SendNotification(std::string notification)
{
	np_error_t result = np_post_notification(this->client(), notification.c_str());
	if (result != NP_E_SUCCESS)
	{
		throw ServerError(ServerErrorCode::LostConnection);
	}
}

std::shared_ptr<Device> NotificationConnection::device() const
{
	return _device;
}

np_client_t NotificationConnection::client() const
{
	return _client;
}

std::function<void(std::string)> NotificationConnection::receivedNotificationHandler() const
{
	return _receivedNotificationHandler;
}

void NotificationConnection::setReceivedNotificationHandler(std::function<void(std::string)> handler)
{
	_receivedNotificationHandler = handler;
}

bool NotificationConnection::active() const
{
	return _active;
}
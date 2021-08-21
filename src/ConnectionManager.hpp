//
//  ConnectionManager.hpp
//  AltServer-Windows
//
//  Created by Riley Testut on 8/13/19.
//  Copyright © 2019 Riley Testut. All rights reserved.
//

#ifndef ConnectionManager_hpp
#define ConnectionManager_hpp

#include <vector>
#include <set>
#include <thread>
#include <map>

#include "ClientConnection.h"
#include "NotificationConnection.h"

class ConnectionManager
{
public:
	static ConnectionManager* instance();

	void Start();
	void Disconnect(std::shared_ptr<ClientConnection> connection);

private:
	ConnectionManager();
	~ConnectionManager();

	static ConnectionManager* _instance;

	std::thread _listeningThread;

	int _mDNSResponderSocket;
	std::set<std::shared_ptr<ClientConnection>> _connections;
	std::map<std::string, std::shared_ptr<NotificationConnection>> _notificationConnections;

	int mDNSResponderSocket() const;
	std::set<std::shared_ptr<ClientConnection>> connections() const;
	std::map<std::string, std::shared_ptr<NotificationConnection>> notificationConnections() const;

	void Listen();
	void StartAdvertising(int port);

	void StartNotificationConnection(std::shared_ptr<Device> device);
	void StopNotificationConnection(std::shared_ptr<Device> device);

	void HandleRequest(std::shared_ptr<ClientConnection> connection);
	void HandleNotification(std::string notification, std::shared_ptr<NotificationConnection> connection);

	friend void ConnectionManagerConnectedDevice(std::shared_ptr<Device> device);
	friend void ConnectionManagerDisconnectedDevice(std::shared_ptr<Device> device);
};

#endif /* ConnectionManager_hpp */

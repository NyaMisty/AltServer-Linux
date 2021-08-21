//
//  ConnectionManager.cpp
//  AltServer-Windows
//
//  Created by Riley Testut on 8/13/19.
//  Copyright © 2019 Riley Testut. All rights reserved.
//

#include "ConnectionManager.hpp"

#include <iostream>
#include <thread>

//#include <netinet/in.h>

#include <stddef.h>

/*
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/socket.h>
*/

#ifdef HAVE_MDNS
#include "dns_sd.h"
#endif

#include "AltServerApp.h"
#include "WirelessConnection.h"
#include "DeviceManager.hpp"
#include "Error.hpp"

#include <memory>

#define WIRED_SERVER_CONNECTION_AVAILABLE_REQUEST "io.altstore.Request.WiredServerConnectionAvailable"
#define WIRED_SERVER_CONNECTION_AVAILABLE_RESPONSE "io.altstore.Response.WiredServerConnectionAvailable"
#define WIRED_SERVER_CONNECTION_START_REQUEST "io.altstore.Request.WiredServerConnectionStart"

#ifdef HAVE_MDNS
void DNSSD_API ConnectionManagerBonjourRegistrationFinished(DNSServiceRef service, DNSServiceFlags flags, DNSServiceErrorType errorCode, const char *name, const char *regtype, const char *domain, void *context)
{
	std::cout << "Registered service: " << name << " (Error: " << errorCode << ")" << std::endl;
}
#endif

void ConnectionManagerConnectedDevice(std::shared_ptr<Device> device)
{
	ConnectionManager::instance()->StartNotificationConnection(device);
}

void ConnectionManagerDisconnectedDevice(std::shared_ptr<Device> device)
{
	ConnectionManager::instance()->StopNotificationConnection(device);
}

ConnectionManager* ConnectionManager::_instance = nullptr;

ConnectionManager* ConnectionManager::instance()
{
    if (_instance == 0)
    {
        _instance = new ConnectionManager();
    }
    
    return _instance;
}

ConnectionManager::ConnectionManager()
{
	DeviceManager::instance()->setConnectedDeviceCallback(ConnectionManagerConnectedDevice);
	DeviceManager::instance()->setDisconnectedDeviceCallback(ConnectionManagerDisconnectedDevice);
}

void ConnectionManager::Start()
{
    auto listenFunction = [](void) {
        ConnectionManager::instance()->Listen();
    };
    
    _listeningThread = std::thread(listenFunction);
}

void ConnectionManager::Disconnect(std::shared_ptr<ClientConnection> connection)
{
	connection->Disconnect();
	_connections.erase(connection);
}

void ConnectionManager::StartAdvertising(int socketPort)
{
#ifdef HAVE_MDNS
	DNSServiceRef service = NULL;
	uint16_t port = htons(socketPort);

	auto serverID = AltServerApp::instance()->serverID();

	std::string txtValue("serverID=" + serverID);
	char size = txtValue.size();

	std::vector<char> txtData;
	txtData.reserve(size + 1);
	txtData.push_back(size);

	for (auto& byte : txtValue)
	{
		txtData.push_back(byte);
	}

	DNSServiceErrorType registrationResult = DNSServiceRegister(&service, 0, 0, NULL, "_altserver._tcp", NULL, NULL, port, txtData.size(), txtData.data(), ConnectionManagerBonjourRegistrationFinished, NULL);
	if (registrationResult != kDNSServiceErr_NoError)
	{
		std::cout << "Bonjour Registration Error: " << registrationResult << std::endl;
		return;
	}

	int dnssd_socket = DNSServiceRefSockFD(service);
	if (dnssd_socket == -1)
	{
		std::cout << "Failed to retrieve mDNSResponder socket." << std::endl;
	}

	this->_mDNSResponderSocket = dnssd_socket;
#endif
}

void ConnectionManager::Listen()
{    
    int socket4 = socket(AF_INET, SOCK_STREAM, 0);
    if (socket4 == 0)
    {
        std::cout << "Failed to create socket." << std::endl;
        return;
    }
    
    struct sockaddr_in address4;
    memset(&address4, 0, sizeof(address4));
    //address4.sin_len = sizeof(address4);
    address4.sin_family = AF_INET;
    address4.sin_port = 0; // Choose for us.
    address4.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(socket4, (struct sockaddr *)&address4, sizeof(address4)) < 0)
    {
        std::cout << "Failed to bind socket." << std::endl;
        return;
    }
    
    if (listen(socket4, 0) != 0)
    {
        std::cout << "Failed to prepare listening socket." << std::endl;
    }
    
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    
    if (getsockname(socket4, (struct sockaddr *)&sin, &len) == -1)
    {
        std::cout << "Failed to get socket name." << std::endl;
    }
    
    int port4 = ntohs(sin.sin_port);
    this->StartAdvertising(port4);
    
    fd_set input_set;
    fd_set copy_set;
    
    while (true)
    {
        struct timeval tv;
        tv.tv_sec = 1; /* 1 second timeout */
        tv.tv_usec = 0; /* no microseconds. */
        
        /* Selection */
        FD_ZERO(&input_set );   /* Empty the FD Set */
        FD_SET(socket4, &input_set);  /* Listen to the input descriptor */
        
        FD_ZERO(&copy_set );   /* Empty the FD Set */
        FD_SET(socket4, &copy_set);  /* Listen to the input descriptor */
        
        int ready_for_reading = select(socket4 + 1, &input_set, &copy_set, NULL, &tv);
        
        /* Selection handling */
        if (ready_for_reading > 0)
        {
			
            
			struct sockaddr_in clientAddress;
            memset(&clientAddress, 0, sizeof(clientAddress));

			socklen_t addrlen = sizeof(clientAddress);
            int other_socket = accept(socket4, (struct sockaddr *)&clientAddress, &addrlen);
            
			
            char *ipaddress = inet_ntoa(((struct sockaddr_in)clientAddress).sin_addr);
            int port2 = ntohs(((struct sockaddr_in)clientAddress).sin_port);
			//int error = WSAGetLastError();
			int error = errno;

			odslog("Other Socket:" << other_socket << ". Port: " << port2 << ". Error: " << error);
            
			std::shared_ptr<ClientConnection> clientConnection(new WirelessConnection(other_socket));
			this->HandleRequest(clientConnection);
        }
        else if (ready_for_reading == -1)
        {
             /* Handle the error */
            std::cout << "Uh-oh" << std::endl;
        }
        else
        {
           // Do nothing
        }
    }
}

void ConnectionManager::StartNotificationConnection(std::shared_ptr<Device> device)
{
	odslog("Starting notification connection to device: " << device->name().c_str());

	DeviceManager::instance()->StartNotificationConnection(device)
		.then([=](pplx::task<std::shared_ptr<NotificationConnection>> task) {
		std::vector<std::string> notifications = { WIRED_SERVER_CONNECTION_AVAILABLE_REQUEST, WIRED_SERVER_CONNECTION_START_REQUEST };

		try
		{
			auto connection = task.get();

			connection->StartListening(notifications);
			connection->setReceivedNotificationHandler([=](std::string notification) {
				this->HandleNotification(notification, connection);
			});

			this->_notificationConnections[device->identifier()] = connection;
		}
		catch (Error& e)
		{
			odslog("Failed to start notification connection. " << e.localizedDescription().c_str());
		}
		catch (std::exception& e)
		{
			odslog("Failed to start notification connection. " << e.what());
		}
	});
}

void ConnectionManager::StopNotificationConnection(std::shared_ptr<Device> device)
{
	if (this->notificationConnections().count(device->identifier()) == 0)
	{
		return;
	}

	auto connection = this->notificationConnections()[device->identifier()];
	connection->Disconnect();

	this->_notificationConnections.erase(device->identifier());
}

void ConnectionManager::HandleNotification(std::string notification, std::shared_ptr<NotificationConnection> connection)
{
	if (notification == WIRED_SERVER_CONNECTION_AVAILABLE_REQUEST)
	{
		try
		{
			connection->SendNotification(WIRED_SERVER_CONNECTION_AVAILABLE_RESPONSE);

			odslog("Sent wired server connection available response!");
		}
		catch (Error& e)
		{
			odslog("Error sending wired server connection response. " << e.localizedDescription().c_str());
		}
		catch (std::exception& e)
		{
			odslog("Error sending wired server connection response. " << e.what());
		}
	}
	else if (notification == WIRED_SERVER_CONNECTION_START_REQUEST)
	{
		DeviceManager::instance()->StartWiredConnection(connection->device())
			.then([=](pplx::task<std::shared_ptr<WiredConnection>> task) {
			try
			{
				auto wiredConnection = task.get();
				auto connection = std::dynamic_pointer_cast<ClientConnection>(wiredConnection);

				odslog("Started wired server connection!");

				this->HandleRequest(wiredConnection);
			}
			catch (Error& e)
			{
				odslog("Error starting wired server connection. " << e.localizedDescription().c_str());
			}
			catch (std::exception& e)
			{
				odslog("Error starting wired server connection. " << e.what());
			}
		});
	}
}

void ConnectionManager::HandleRequest(std::shared_ptr<ClientConnection> clientConnection)
{
	this->_connections.insert(clientConnection);

	clientConnection->ProcessAppRequest().then([=](pplx::task<void> task) {
		try
		{
			task.get();

			odslog("Finished handling request!");
		}
		catch (Error& e)
		{
			odslog("Failed to handle request:" << e.localizedDescription().c_str());
		}
		catch (std::exception& e)
		{
			odslog("Failed to handle request:" << e.what());
		}
		
		this->Disconnect(clientConnection);
	});
}

int ConnectionManager::mDNSResponderSocket() const
{
    return _mDNSResponderSocket;
}

std::set<std::shared_ptr<ClientConnection>> ConnectionManager::connections() const
{
    return _connections;
}

std::map<std::string, std::shared_ptr<NotificationConnection>> ConnectionManager::notificationConnections() const
{
	return _notificationConnections;
}

#pragma once

#include "ClientConnection.h"

class WirelessConnection: public ClientConnection
{
public:
	WirelessConnection(int socket);
	virtual ~WirelessConnection();

	virtual void Disconnect();

	virtual pplx::task<void> SendData(std::vector<unsigned char>& data);
	virtual pplx::task<std::vector<unsigned char>> ReceiveData(int size);

	int socket() const;

private:
	int _socket;
};


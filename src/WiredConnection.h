#pragma once

#include "Device.hpp"
#include "ClientConnection.h"

#include <pplx/pplxtasks.h>
#include <libimobiledevice/libimobiledevice.h>

#include <memory>

class WiredConnection: public ClientConnection
{
public:
	WiredConnection(std::shared_ptr<Device> device, idevice_connection_t connection);
	virtual ~WiredConnection();

	virtual void Disconnect();

	virtual pplx::task<void> SendData(std::vector<unsigned char>& data);
	virtual pplx::task<std::vector<unsigned char>> ReceiveData(int expectedSize);

	std::shared_ptr<Device> device() const;

private:
	std::shared_ptr<Device> _device;
	idevice_connection_t _connection;

	idevice_connection_t connection() const;
};


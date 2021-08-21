#include "WiredConnection.h"
#include "ServerError.hpp"

WiredConnection::WiredConnection(std::shared_ptr<Device> device, idevice_connection_t connection) : _device(device), _connection(connection)
{
}

WiredConnection::~WiredConnection()
{
}

void WiredConnection::Disconnect()
{
	if (this->connection() == NULL)
	{
		return;
	}

	idevice_disconnect(this->connection());
	_connection = NULL;
}

pplx::task<void> WiredConnection::SendData(std::vector<unsigned char>& data)
{
	return pplx::create_task([&data, this]() {
		while (data.size() > 0)
		{
			uint32_t sentBytes = 0;
			if (idevice_connection_send(this->connection(), (const char*)data.data(), (int32_t)data.size(), &sentBytes) != IDEVICE_E_SUCCESS)
			{
				throw ServerError(ServerErrorCode::LostConnection);
			}

			data.erase(data.begin(), data.begin() + sentBytes);
		}
	});
}

pplx::task<std::vector<unsigned char>> WiredConnection::ReceiveData(int expectedSize)
{
	return pplx::create_task([=]() -> std::vector<unsigned char> {
		char bytes[4096];

		std::vector<unsigned char> receivedData;
		receivedData.reserve(expectedSize);

		while (receivedData.size() < expectedSize)
		{
			uint32_t size = std::min((uint32_t)4096, (uint32_t)expectedSize - (uint32_t)receivedData.size());

			uint32_t receivedBytes = 0;
			idevice_error_t result = idevice_connection_receive_timeout(this->connection(), bytes, size, &receivedBytes, 0);
			if (result != IDEVICE_E_SUCCESS)
			{
				throw ServerError(ServerErrorCode::LostConnection);
			}

			receivedData.insert(receivedData.end(), bytes, bytes + receivedBytes);
		}

		return receivedData;
	});
}

std::shared_ptr<Device> WiredConnection::device() const
{
	return _device;
}

idevice_connection_t WiredConnection::connection() const
{
	return _connection;
}

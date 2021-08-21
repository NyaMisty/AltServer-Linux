#include "WirelessConnection.h"
#include <algorithm>
using std::min;
#include <stdlib.h>
#include <cpprest/json.h>

#include "ServerError.hpp"

#if SIZE_MAX == UINT_MAX
typedef int ssize_t;        /* common 32 bit case */
#elif SIZE_MAX == ULONG_MAX
typedef long ssize_t;       /* linux 64 bits */
#elif SIZE_MAX == ULLONG_MAX
typedef long long ssize_t;  /* windows 64 bits */
#elif SIZE_MAX == USHRT_MAX
typedef short ssize_t;      /* is this even possible? */
#else
#error platform has exotic SIZE_MAX
#endif

WirelessConnection::WirelessConnection(int socket) : _socket(socket)
{
}

WirelessConnection::~WirelessConnection()
{
}

void WirelessConnection::Disconnect()
{
	if (this->socket() == 0)
	{
		return;
	}

	closesocket(this->socket());
	_socket = 0;
}

pplx::task<void> WirelessConnection::SendData(std::vector<unsigned char>& data)
{
	return pplx::create_task([this, data]() {
		fd_set input_set;
		fd_set copy_set;

		int64_t totalSentBytes = 0;

		while (true)
		{
			struct timeval tv;
			tv.tv_sec = 1; /* 1 second timeout */
			tv.tv_usec = 0; /* no microseconds. */

			/* Selection */
			FD_ZERO(&input_set);   /* Empty the FD Set */
			FD_SET(this->socket(), &input_set);  /* Listen to the input descriptor */

			FD_ZERO(&copy_set);   /* Empty the FD Set */
			FD_SET(this->socket(), &copy_set);  /* Listen to the input descriptor */

			ssize_t sentBytes = send(this->socket(), (const char*)data.data(), (size_t)(data.size() - totalSentBytes), 0);
			totalSentBytes += sentBytes;

			std::cout << "Sent Bytes Count: " << sentBytes << " (" << totalSentBytes << ")" << std::endl;

			if (totalSentBytes >= sentBytes)
			{
				break;
			}
		}

		std::cout << "Sent Data: " << totalSentBytes << " Bytes" << std::endl;
		});
}

pplx::task<std::vector<unsigned char>> WirelessConnection::ReceiveData(int size)
{
	return pplx::create_task([this, size]() {
		std::vector<unsigned char> data;
		data.reserve(size);

		char buffer[4096];

		fd_set          input_set;
		fd_set          copy_set;

		while (true)
		{
			struct timeval tv;
			tv.tv_sec = 1; /* 1 second timeout */
			tv.tv_usec = 0; /* no microseconds. */

			int socket = this->socket();
			std::cout << "Checking socket: " << socket << std::endl;

			/* Selection */
			FD_ZERO(&input_set);   /* Empty the FD Set */
			FD_SET(socket, &input_set);  /* Listen to the input descriptor */

			FD_ZERO(&copy_set);   /* Empty the FD Set */
			FD_SET(socket, &copy_set);  /* Listen to the input descriptor */

			int result = select(this->socket() + 1, &input_set, &copy_set, NULL, &tv);

			if (result == 0)
			{
				continue;
			}
			else if (result == -1)
			{
				std::cout << "Error!" << std::endl;
			}
			else
			{
				ssize_t readBytes = recv(this->socket(), buffer, min((ssize_t)4096, (ssize_t)(size - data.size())), 0);
				for (int i = 0; i < readBytes; i++)
				{
					data.push_back(buffer[i]);
				}

				odslog("Received bytes: " << data.size() << "(of " << size << ")");

				if (data.size() >= size)
				{
					break;
				}
			}
		}

		return data;
	});
}

int WirelessConnection::socket() const
{
	return _socket;
}
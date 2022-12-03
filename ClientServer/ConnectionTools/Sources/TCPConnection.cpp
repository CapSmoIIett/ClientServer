#include "../Headers/TCPConnection.h"

#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>

#define MEOF "EOF"
#define MSG_SIZE KB


TCPClient::TCPClient() :
#if defined(OS_WINDOWS)
	m_pWsaData(),
#endif
//	m_pAddrInfo(nullptr),
	m_sConnectionSocket(INVALID_SOCKET)
{

}

bool TCPClient::Start()
{
#if defined(OS_WINDOWS)
	m_ConnectionAddress.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
#else
	m_ConnectionAddress.sin_addr.s_addr = INADDR_ANY;
#endif
	m_ConnectionAddress.sin_family = AF_INET;
	m_ConnectionAddress.sin_port = htons(2000);

#if defined(OS_WINDOWS)
	if (WSAStartup(MAKEWORD(2, 2), &m_pWsaData) != 0)
		return false;
#endif

	return true;
}

bool TCPClient::Connect(const char* ip)
{
	int result = 0;

	m_sConnectionSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sConnectionSocket == INVALID_SOCKET)
	{
#if defined(OS_WINDOWS)
		printf("Error: Invalid Socket (%d)\n", WSAGetLastError());
#endif
		return false;
	}

	if (ip != nullptr)
#if defined(OS_WINDOWS)
		m_ConnectionAddress.sin_addr.S_un.S_addr = inet_addr(ip);
#else
		inet_aton(ip, &m_ConnectionAddress.sin_addr);
#endif

	result = connect(m_sConnectionSocket, 
		(struct sockaddr*)&m_ConnectionAddress, 
		sizeof(m_ConnectionAddress));
	if (result != 0)
	{
#if defined(OS_WINDOWS)
		printf("Error: Invalid Connection (%d)\n", WSAGetLastError());
#endif
		return false;
	}	


	std::cout << "connected";

	return true;
}


bool TCPClient::ShutdownProcess()
{
	/*if (m_pAddrInfo != nullptr)
	{
		freeaddrinfo(m_pAddrInfo);
		m_pAddrInfo = nullptr;
	}*/

#if defined(OS_WINDOWS)
	WSACleanup();
#endif

	if (m_sConnectionSocket != INVALID_SOCKET)
	{
		shutdown(m_sConnectionSocket, SD_BOTH);


#if defined(OS_WINDOWS)
		closesocket(m_sConnectionSocket);
#else
		close(m_sConnectionSocket);
#endif
		m_sConnectionSocket = INVALID_SOCKET;
	}

	return false; 
}


std::string TCPClient::Get()
{
	int amountBytes = 0;
	char recvBuffer[512];
	std::string result;

	do
	{
#if defined(OS_WINDOWS)
		ZeroMemory(recvBuffer, sizeof(recvBuffer));
#else
		memset(recvBuffer, 0, sizeof(recvBuffer));
#endif

#if defined(OS_WINDOWS)
		u_long t = true; 
		ioctlsocket(m_sConnectionSocket, FIONBIO, &t);

		amountBytes = recv(m_sConnectionSocket, recvBuffer, sizeof(recvBuffer), 0);

		t = false;
		ioctlsocket(m_sConnectionSocket, FIONBIO, &t);
#else
		amountBytes = recv(m_sConnectionSocket, recvBuffer, sizeof(recvBuffer), MSG_DONTWAIT);
#endif

		result += std::string(recvBuffer);
	} while (amountBytes > sizeof(recvBuffer));

	return result;
}

bool TCPClient::Send(std::string msg)
{

#if defined(OS_WINDOWS)
	if (send(m_sConnectionSocket, msg.c_str(), msg.size(), 0) == SOCKET_ERROR)
		return ShutdownProcess();
#else 
	if (send(m_sConnectionSocket, msg.c_str(), msg.size(), 0) < 0)
		return ShutdownProcess();
#endif 

	return true;
}

bool TCPClient::Disconnect()
{
	auto iResult = shutdown(m_sConnectionSocket, SD_SEND);
#if defined(OS_WINDOWS)
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed: %d\n", WSAGetLastError());
		WSACleanup();
		closesocket(m_sConnectionSocket);
#else
	if (iResult < 0) {
		close(m_sConnectionSocket);
#endif
		return 1;
	}
	return false;
}

bool TCPClient::SendFile(std::fstream& file)
{
	char buffer[MSG_SIZE]; //выделяем блок 1 Кб
	int sended = 0;
	int readed = 0;

	if (!file.is_open())
		return false;

	file.read(buffer, sizeof(buffer));
	while ((readed = file.gcount()) != 0)
	{
		sended = send(m_sConnectionSocket, (char*)buffer, readed, 0);
	
		if (sended != readed)
			std::cout << "ERROR" << "\n";

		file.read(buffer, sizeof(buffer));
	}

#if defined(OS_WINDOWS)
	Sleep(1000);
#else
	sleep(1000);
#endif

	send(m_sConnectionSocket, (char*)MEOF, sizeof(MEOF), 0);

	return true;
}

bool TCPClient::GetFile(std::fstream& file)
{
	char buffer[MSG_SIZE]; //выделяем блок 1 Кб
	int len = 0;

	if (!file.is_open())
		return false;

	do
	{
#if defined(OS_WINDOWS)
		u_long t = true; 
		ioctlsocket(m_sConnectionSocket, FIONBIO, &t);

		len = recv(m_sConnectionSocket, (char*)buffer, sizeof(buffer), 0);

		t = false;
		ioctlsocket(m_sConnectionSocket, FIONBIO, &t);
#else
		len = recv(m_sConnectionSocket, (char*)buffer, sizeof(buffer), MSG_DONTWAIT);
#endif

		if (strcmp(buffer, MEOF) == 0)
			break;

		file.write(buffer, len);
	} while (len != 0);

	return true;
}


//-----------------------------------------------------------------------------------------------


TCPServer::TCPServer() :
#if defined(OS_WINDOWS)
	m_pWsaData(),
#endif
	//m_pAddrInfo(nullptr),
	m_sConnectionSocket(INVALID_SOCKET)
{
	isClose = false;

}

bool TCPServer::Start()
{
#if defined(OS_WINDOWS)
	m_ServerAddress.sin_addr.S_un.S_addr = INADDR_ANY;
#else
	m_ServerAddress.sin_addr.s_addr = INADDR_ANY;
#endif
	m_ServerAddress.sin_family = AF_INET;
	m_ServerAddress.sin_port = htons(2000);

#if defined(OS_WINDOWS)
	if (WSAStartup(MAKEWORD(2, 2), &m_pWsaData) != 0)
		return false;
#endif

	return true;

}

bool TCPServer::Connect()
{
	int result = 0;

	m_sConnectionSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sConnectionSocket == INVALID_SOCKET)
	{
#if defined(OS_WINDOWS)
		printf("Error: Invalid Socket (%d)\n", WSAGetLastError());
#endif
		return false;
	}

	//struct timeval tv;
	//tv.tv_sec = 30;

	//setsockopt(m_sConnectionSocket, SOL_SOCKET, SO_RCVTIMEO, (struct timeval*)&tv, sizeof(struct timeval));
	//setsockopt(m_sConnectionSocket, SOL_SOCKET, SO_SNDTIMEO, (struct timeval*)&tv, sizeof(struct timeval));

	BOOL iVal = true;
	
	//result = setsockopt(m_sConnectionSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&iVal, sizeof(iVal));
	/*result = setsockopt(m_sConnectionSocket, SOL_SOCKET, SO_SNDTIMEO, (char*)&iVal, sizeof(iVal));
	if (result == SOCKET_ERROR)
	{
		printf("setsockopt() failed with error code %d\n", WSAGetLastError());
	}*/

	result = bind(m_sConnectionSocket, (struct sockaddr*)&m_ServerAddress, sizeof(m_ServerAddress));
#if defined(OS_WINDOWS)
	if (result == SOCKET_ERROR)
	{
		printf("Error: Invalid Bind (%d)\n", WSAGetLastError());
#else
	if (result < 0)
	{
#endif
		return false;
	}

	result = listen(m_sConnectionSocket, 5);
#if defined(OS_WINDOWS)
	if (result == SOCKET_ERROR)
	{
		printf("Error: Invalid Listen (%d)\n", WSAGetLastError());
#else
	if (result < 0)
	{
#endif
		return false;
	}

	return true;
}

ConnectedDevice& TCPServer::Access()
{
#if defined(OS_WINDOWS)
	SOCKADDR_IN cs_addr;
#else 
	sockaddr_in cs_addr;
#endif

	socklen_t cs_addrsize = sizeof(cs_addr);

#if defined(OS_WINDOWS)
	SOCKET ClientSocket = accept(m_sConnectionSocket, (SOCKADDR*) &cs_addr, &cs_addrsize);
#else
	int ClientSocket = accept(m_sConnectionSocket, (sockaddr*) &cs_addr, &cs_addrsize);
#endif


	if (ClientSocket == INVALID_SOCKET)
		return ConnectedDevice(ClientSocket, cs_addr, ConnectedDevice::Status::Disabled);

	int flag = 1;
#ifdef OS_WINDOWS
	tcp_keepalive ka { 1, 10 * 1000, 3 * 1000 };

	setsockopt(ClientSocket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&flag, sizeof(flag));
	//if (setsockopt(ClientSocket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&flag, sizeof(flag)) != 0) 
	//	return false;

	unsigned long numBytesReturned = 0;

	WSAIoctl(ClientSocket, SIO_KEEPALIVE_VALS, &ka, sizeof(ka), nullptr, 0, &numBytesReturned, 0, nullptr);
	//if (WSAIoctl(ClientSocket, SIO_KEEPALIVE_VALS, &ka, sizeof(ka), nullptr, 0, &numBytesReturned, 0, nullptr) != 0) 
	//	return false;
#else //POSIX
	//if (setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag)) == -1) return false;
	//if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPIDLE, &ka_conf.ka_idle, sizeof(ka_conf.ka_idle)) == -1) return false;
	//if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPINTVL, &ka_conf.ka_intvl, sizeof(ka_conf.ka_intvl)) == -1) return false;
	//if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPCNT, &ka_conf.ka_cnt, sizeof(ka_conf.ka_cnt)) == -1) return false;
	int idle = 10;
	int intvl = 3;
	int cnt = 5;

	setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag);
	setsockopt(socket, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(ka_conf.ka_idle);
	setsockopt(socket, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(ka_conf.ka_intvl);
	setsockopt(socket, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(ka_conf.ka_cnt);
#endif


	ConnectedDevice device(ClientSocket, cs_addr, ConnectedDevice::Status::Connected);

	auto connectedDevice = std::find_if(m_vConnectedDevices.begin(), m_vConnectedDevices.end(), [cs_addr](auto second)
		{
			return cs_addr.sin_addr.s_addr == second.m_SockAddr.sin_addr.s_addr;
		});

	if (connectedDevice != m_vConnectedDevices.end())
	{
		device.m_CLInfo = connectedDevice->m_CLInfo;
		*connectedDevice = device;
		std::cout << "client reconect\n";
		return device;
	}

	m_vSockets.push_back(ClientSocket);
	m_vConnectedDevices.push_back(device);

	std::cout << "client connected\n";

	return device;
}

bool TCPServer::ShutdownProcess()
{
	/*if (m_pAddrInfo != nullptr)
	{
		freeaddrinfo(m_pAddrInfo);
		m_pAddrInfo = nullptr;
	}*/

#if defined(OS_WINDOWS)
	WSACleanup();
#endif

	if (m_sConnectionSocket != INVALID_SOCKET)
	{
		shutdown(m_sConnectionSocket, SD_BOTH);
#if defined(OS_WINDOWS)
		closesocket(m_sConnectionSocket);
#else
		close(m_sConnectionSocket);
#endif
		m_sConnectionSocket = INVALID_SOCKET;
	}

	isClose = true;

	for (auto socket : m_vSockets)
	{
		shutdown(socket, SD_BOTH);
#if defined(OS_WINDOWS)
		closesocket(socket);
#else
		close(socket);
#endif
		socket = INVALID_SOCKET;
	}

	return false; 
}

std::string TCPServer::Get(ConnectedDevice& device)
{
	int amountBytes = 0;
	char recvBuffer[512];
	std::string result;

	do
	{
#if defined(OS_WINDOWS)
		ZeroMemory(recvBuffer, sizeof(recvBuffer));
#else
		memset(recvBuffer, 0, sizeof(recvBuffer));
#endif

		amountBytes = recvfrom(device.m_Socket, recvBuffer, sizeof(recvBuffer), 0, 0, 0);
		
#if defined(OS_WINDOWS)
		if (WSAGetLastError() == WSAECONNRESET || WSAGetLastError() == WSAETIMEDOUT)
#else
		if (amountBytes < 0)
#endif
		{
			device.m_Status = ConnectedDevice::Status::Disabled;
			return "";
		}

		result += std::string(recvBuffer);
	} while (amountBytes >= sizeof(recvBuffer));

	return result;
}

bool TCPServer::Send(ConnectedDevice& device, std::string msg)
{
#if defined(OS_WINDOWS)
	if (send(device.m_Socket, msg.c_str(), msg.size(), 0) == SOCKET_ERROR)
		return ShutdownProcess();
#else 
	if (send(device.m_Socket, msg.c_str(), msg.size(), 0) < 0)
		return ShutdownProcess();
#endif 

	return false;
}

bool TCPServer::SendFile(ConnectedDevice& device, std::fstream& file)
{
	char buffer[MSG_SIZE]; //выделяем блок 1 Кб
	int readed = 0;
	int counter = 0;
	int sended = 0;

	if (!file.is_open())
		return false;

	std::cout << "\n\n";

	file.read(buffer, sizeof(buffer));
	while ((readed = file.gcount()) != 0)
	{
		auto clock = std::chrono::high_resolution_clock::now();

		sended = send(device.m_Socket, (char*)buffer, readed, 0);
		file.read(buffer, sizeof(buffer));

#if defined(OS_WINDOWS)
		if (WSAGetLastError() == WSAECONNRESET)
#else
		if (sended < 0)
#endif
		{
			device.m_Status = ConnectedDevice::Status::Disabled;
			device.m_CLInfo = ConnectionLostInfo(ConnectionLostInfo::Status::download, counter * MSG_SIZE, "");
			return false;
		}

		counter++;
		auto nanosec = clock.time_since_epoch();
		//std::cout << buffer << "\n";
		std::cout << MSG_SIZE / (static_cast<double>(nanosec.count()) / (1000000000.0)) << "\n";
	}

#if defined(OS_WINDOWS)
	Sleep(1000);
#else
	sleep(1000);
#endif

	send(device.m_Socket, (char*)MEOF, sizeof(MEOF), 0);

	return true;
}

bool TCPServer::GetFile(ConnectedDevice& device, std::fstream& file)
{
	char buffer[MSG_SIZE]; //выделяем блок 1 Кб
	int len = 0;
	int counter = 0;

	if (!file.is_open())
		return false;

	do
	{
		auto clock = std::chrono::high_resolution_clock::now();

		len = recv(device.m_Socket, (char*)buffer, sizeof(buffer), 0);

		if (strcmp(buffer, MEOF) == 0)
			break;

		file.write(buffer, len);

#if defined(OS_WINDOWS)
		if (WSAGetLastError() == WSAECONNRESET)
#else
		if (len < 0)
#endif
		{
			device.m_Status = ConnectedDevice::Status::Disabled;
			device.m_CLInfo = ConnectionLostInfo(ConnectionLostInfo::Status::upload, counter * MSG_SIZE, "");
			return false;
		}

		if (len < sizeof(buffer))
			std::cout << "ERROR" << "\n";

		counter++;
		auto nanosec = clock.time_since_epoch();
		std::cout << MSG_SIZE * 8 / (static_cast<double>(nanosec.count()) / (1000000000.0)) << "\n";
	} while (len > 0);

	return true;
}

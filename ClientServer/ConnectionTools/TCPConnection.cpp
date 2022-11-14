#include "TCPConnection.h"

#include <iostream>


TCPClient::TCPClient() :
	m_pWsaData(),
	m_pAddrInfo(nullptr),
	m_sConnectionSocket(INVALID_SOCKET)
{

}

bool TCPClient::Start()
{
	if (WSAStartup(MAKEWORD(2, 2), &m_pWsaData) != 0)
		return false;

}

bool TCPClient::Connect(const char* ip)
{
	ADDRINFO hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;			//IPv4
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	ADDRINFO* addrinfo;
	// подключаемся к IP
	GetPort() += 1;
	if (getaddrinfo(ip, "8080", &hints, &addrinfo) != 0)
		;//	return ShutdownProcess();

	//addrResult->ai_next - если несколько мест куда можно подключиться

	SOCKADDR_IN addr;
	addr.sin_addr.s_addr = inet_addr(ip); //"127.0.0.1");
	addr.sin_port = htons(8080);
	addr.sin_family = AF_INET;

	//int result = bind(m_sConnectionSocket, (SOCKADDR*)&addr, sizeof(addr));
	// получения сокета
	//m_sConnectionSocket = socket(m_pAddrInfo->ai_family, m_pAddrInfo->ai_socktype, m_pAddrInfo->ai_protocol);
	m_sConnectionSocket = socket(AF_INET, SOCK_STREAM, NULL);
	if (m_sConnectionSocket == INVALID_SOCKET)
		return ShutdownProcess();

	//int result = connect(m_sConnectionSocket, (SOCKADDR*)&addr, sizeof(addr));
	int result = connect(m_sConnectionSocket, addrinfo->ai_addr, addrinfo->ai_addrlen);
	if (result == SOCKET_ERROR)
		return ShutdownProcess();

	std::cout << "connected";

	return true;
}


bool TCPClient::ShutdownProcess()
{
	if (m_pAddrInfo != nullptr)
	{
		freeaddrinfo(m_pAddrInfo);
		m_pAddrInfo = nullptr;
	}

	WSACleanup();

	if (m_sConnectionSocket != INVALID_SOCKET)
	{
		shutdown(m_sConnectionSocket, SD_BOTH);
		closesocket(m_sConnectionSocket);
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
		ZeroMemory(recvBuffer, sizeof(recvBuffer));

		amountBytes = recv(m_sConnectionSocket, recvBuffer, sizeof(recvBuffer), 0);

		result += std::string(recvBuffer);
	} while (amountBytes > sizeof(recvBuffer));

	return result;
}

bool TCPClient::Send(std::string msg)
{
	if (send(m_sConnectionSocket, msg.c_str(), msg.size(), NULL) == SOCKET_ERROR)
		return ShutdownProcess();

	std::cout << "sended\n";

}

bool TCPClient::Disconnect()
{
	auto iResult = shutdown(m_sConnectionSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed: %d\n", WSAGetLastError());
		closesocket(m_sConnectionSocket);
		WSACleanup();
		return 1;
	}
}

bool TCPClient::SendFile(std::fstream& file)
{
	char buffer[1024]; //выделяем блок 1 Кб
	int readed = 0;

	if (!file.is_open())
		return false;

	file.read(buffer, sizeof(buffer));
	while ((readed = file.gcount()) != 0)
	{
		send(m_sConnectionSocket, (char*)buffer, readed, 0);
		file.read(buffer, sizeof(buffer));
	}

	return true;
}

bool TCPClient::GetFile(std::fstream& file)
{
	char buffer[1024]; //выделяем блок 1 Кб
	int len = 0;

	if (!file.is_open())
		return false;

	do
	{
		len = recv(m_sConnectionSocket, (char*)buffer, sizeof(buffer), 0);
		file.write(buffer, len);
	} while (len == sizeof(buffer));

	return true;
}


//-----------------------------------------------------------------------------------------------


TCPServer::TCPServer() :
	m_pWsaData(),
	m_pAddrInfo(nullptr),
	m_sConnectionSocket(INVALID_SOCKET)
{
	isClose = false;

}

bool TCPServer::Start()
{
	if (WSAStartup(MAKEWORD(2, 2), &m_pWsaData) != 0)
		return false;

}

bool TCPServer::Connect()
{
	ADDRINFO hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;			//IPv4
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// подключаемся к IP
	if (getaddrinfo(NULL, GetPortString().c_str(), &hints, &m_pAddrInfo))
		return ShutdownProcess();

	//addrResult->ai_next - если несколько мест куда можно подключиться

	// получения сокета
	//m_sConnectionSocket = socket(m_pAddrInfo->ai_family, m_pAddrInfo->ai_socktype, m_pAddrInfo->ai_protocol);
	m_sConnectionSocket = socket(AF_INET, SOCK_STREAM, NULL);
	if (m_sConnectionSocket == INVALID_SOCKET)
		return ShutdownProcess();

	SOCKADDR_IN addr;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(8080);
	addr.sin_family = AF_INET;

	//int result = bind(m_sConnectionSocket, m_pAddrInfo->ai_addr, m_pAddrInfo->ai_addrlen);
	int result = bind(m_sConnectionSocket, (SOCKADDR*)&addr, sizeof(addr));
	if (result == SOCKET_ERROR)
		return ShutdownProcess();

	if (listen(m_sConnectionSocket, SOMAXCONN) == SOCKET_ERROR)
		return ShutdownProcess();

	if (isClose)
		return true;

	return false;
}

ConnectedDevice& TCPServer::Access()
{
	SOCKADDR_IN cs_addr;
	socklen_t cs_addrsize = sizeof(cs_addr);
	SOCKET ClientSocket = accept(m_sConnectionSocket, (SOCKADDR*) &cs_addr, &cs_addrsize);

	if (ClientSocket == INVALID_SOCKET)
		return ConnectedDevice(ClientSocket, cs_addr, ConnectedDevice::Status::Disabled);

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
	if (m_pAddrInfo != nullptr)
	{
		freeaddrinfo(m_pAddrInfo);
		m_pAddrInfo = nullptr;
	}

	WSACleanup();

	if (m_sConnectionSocket != INVALID_SOCKET)
	{
		shutdown(m_sConnectionSocket, SD_BOTH);
		closesocket(m_sConnectionSocket);
		m_sConnectionSocket = INVALID_SOCKET;
	}

	isClose = true;

	for (auto socket : m_vSockets)
	{
		shutdown(socket, SD_BOTH);
		closesocket(socket);
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
		ZeroMemory(recvBuffer, sizeof(recvBuffer));

		amountBytes = recv(device.m_Socket, recvBuffer, sizeof(recvBuffer), 0);

		if (WSAGetLastError() == WSAECONNRESET)
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
	if (send(device.m_Socket, msg.c_str(), msg.size(), NULL) == SOCKET_ERROR)
		return ShutdownProcess();

	std::cout << "sended\n";

}

bool TCPServer::SendFile(ConnectedDevice& device, std::fstream& file)
{
	char buffer[1024]; //выделяем блок 1 Кб
	int readed = 0;
	int counter = 0;

	if (!file.is_open())
		return false;

	file.read(buffer, sizeof(buffer));
	while ((readed = file.gcount()) != 0)
	{
		auto clock = std::chrono::high_resolution_clock::now();

		send(device.m_Socket, (char*)buffer, readed, 0);
		file.read(buffer, sizeof(buffer));

		if (WSAGetLastError() == WSAECONNRESET)
		{
			device.m_Status = ConnectedDevice::Status::Disabled;
			device.m_CLInfo = ConnectionLostInfo(ConnectionLostInfo::Status::download, counter * 1024, "");
			return false;
		}

		counter++;
		auto nanosec = clock.time_since_epoch();
		std::cout << 1024 / (static_cast<double>(nanosec.count()) / (1000000000.0)) << "\n";
	}

	return true;
}

bool TCPServer::GetFile(ConnectedDevice& device, std::fstream& file)
{
	char buffer[1024]; //выделяем блок 1 Кб
	int len = 0;
	int counter = 0;

	if (!file.is_open())
		return false;

	do
	{
		auto clock = std::chrono::high_resolution_clock::now();

		len = recv(device.m_Socket, (char*)buffer, sizeof(buffer), 0);
		file.write(buffer, len);

		if (WSAGetLastError() == WSAECONNRESET)
		{
			device.m_Status = ConnectedDevice::Status::Disabled;
			device.m_CLInfo = ConnectionLostInfo(ConnectionLostInfo::Status::upload, counter * 1024, "");
			return false;
		}

		counter++;
		auto nanosec = clock.time_since_epoch();
		std::cout << 1024 / (static_cast<double>(nanosec.count()) / (1000000000.0)) << "\n";
	} while (len == sizeof(buffer));

	return true;
}

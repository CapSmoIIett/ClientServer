#include "../Headers/TCPConnection.h"

#include <iostream>

#define EOF "EOFEOF"


TCPClient::TCPClient() :
	m_pWsaData(),
//	m_pAddrInfo(nullptr),
	m_sConnectionSocket(INVALID_SOCKET)
{

}

bool TCPClient::Start()
{
	m_ConnectionAddress.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	m_ConnectionAddress.sin_family = AF_INET;
	m_ConnectionAddress.sin_port = htons(2000);

	if (WSAStartup(MAKEWORD(2, 2), &m_pWsaData) != 0)
		return false;

}

bool TCPClient::Connect(const char* ip)
{
	int result = 0;

	m_sConnectionSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sConnectionSocket == INVALID_SOCKET)
	{
		printf("Error: Invalid Socket (%d)\n", WSAGetLastError());
		return false;
	}

	if (ip != nullptr)
		m_ConnectionAddress.sin_addr.S_un.S_addr = inet_addr(ip);

	result = connect(m_sConnectionSocket, 
		(struct sockaddr*)&m_ConnectionAddress, 
		sizeof(m_ConnectionAddress));
	if (result != 0)
	{
		printf("Error: Invalid Connection (%d)\n", WSAGetLastError());
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
	char buffer[KB]; //выделяем блок 1 Кб
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

	Sleep(100);

	send(m_sConnectionSocket, (char*)EOF, sizeof(EOF), 0);

	return true;
}

bool TCPClient::GetFile(std::fstream& file)
{
	char buffer[KB]; //выделяем блок 1 Кб
	int len = 0;

	if (!file.is_open())
		return false;

	do
	{
		len = recv(m_sConnectionSocket, (char*)buffer, sizeof(buffer), 0);

		if (strcmp(buffer, EOF) == 0)
			break;

		file.write(buffer, len);
	} while (len != 0);

	return true;
}


//-----------------------------------------------------------------------------------------------


TCPServer::TCPServer() :
	m_pWsaData(),
	//m_pAddrInfo(nullptr),
	m_sConnectionSocket(INVALID_SOCKET)
{
	isClose = false;

}

bool TCPServer::Start()
{
	m_ServerAddress.sin_addr.S_un.S_addr = INADDR_ANY;
	m_ServerAddress.sin_family = AF_INET;
	m_ServerAddress.sin_port = htons(2000);

	if (WSAStartup(MAKEWORD(2, 2), &m_pWsaData) != 0)
		return false;

}

bool TCPServer::Connect()
{
	int result = 0;

	m_sConnectionSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sConnectionSocket == INVALID_SOCKET)
	{
		printf("Error: Invalid Socket (%d)\n", WSAGetLastError());
		return false;
	}

	result = bind(m_sConnectionSocket, (struct sockaddr*)&m_ServerAddress, sizeof(m_ServerAddress));
	if (result == SOCKET_ERROR)
	{
		printf("Error: Invalid Bind (%d)\n", WSAGetLastError());
		return false;
	}

	result = listen(m_sConnectionSocket, 5);
	if (result == SOCKET_ERROR)
	{
		printf("Error: Invalid Listen (%d)\n", WSAGetLastError());
		return false;
	}

	return true;
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
	/*if (m_pAddrInfo != nullptr)
	{
		freeaddrinfo(m_pAddrInfo);
		m_pAddrInfo = nullptr;
	}*/

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
	char buffer[KB]; //выделяем блок 1 Кб
	int readed = 0;
	int counter = 0;

	if (!file.is_open())
		return false;

	std::cout << "\n\n";

	file.read(buffer, sizeof(buffer));
	while ((readed = file.gcount()) != 0)
	{
		auto clock = std::chrono::high_resolution_clock::now();

		send(device.m_Socket, (char*)buffer, readed, 0);
		file.read(buffer, sizeof(buffer));

		if (WSAGetLastError() == WSAECONNRESET)
		{
			device.m_Status = ConnectedDevice::Status::Disabled;
			device.m_CLInfo = ConnectionLostInfo(ConnectionLostInfo::Status::download, counter * KB, "");
			return false;
		}

		counter++;
		auto nanosec = clock.time_since_epoch();
		//std::cout << buffer << "\n";
		std::cout << KB * 8 / (static_cast<double>(nanosec.count()) / (1000000000.0)) << "\n";
	}

	Sleep(100);

	send(m_sConnectionSocket, (char*)EOF, sizeof(EOF), 0);

	return true;
}

bool TCPServer::GetFile(ConnectedDevice& device, std::fstream& file)
{
	char buffer[KB]; //выделяем блок 1 Кб
	int len = 0;
	int counter = 0;

	if (!file.is_open())
		return false;

	do
	{
		auto clock = std::chrono::high_resolution_clock::now();

		len = recv(device.m_Socket, (char*)buffer, sizeof(buffer), 0);

		if (strcmp(buffer, EOF) == 0)
			break;

		file.write(buffer, len);

		if (WSAGetLastError() == WSAECONNRESET)
		{
			device.m_Status = ConnectedDevice::Status::Disabled;
			device.m_CLInfo = ConnectionLostInfo(ConnectionLostInfo::Status::upload, counter * KB, "");
			return false;
		}

		if (len < sizeof(buffer))
			std::cout << "ERROR" << "\n";

		counter++;
		auto nanosec = clock.time_since_epoch();
		std::cout << KB * 8 / (static_cast<double>(nanosec.count()) / (1000000000.0)) << "\n";
	} while (len > 0);

	return true;
}

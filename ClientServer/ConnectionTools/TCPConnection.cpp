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

	// подключаемся к IP
	GetPort() += 1;
	if (getaddrinfo(ip, GetPortString().c_str(), &hints, &m_pAddrInfo) != 0)
		;//	return ShutdownProcess();

	//addrResult->ai_next - если несколько мест куда можно подключиться

	SOCKADDR_IN addr;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(1111);
	addr.sin_family = AF_INET;

	//int result = bind(m_sConnectionSocket, (SOCKADDR*)&addr, sizeof(addr));
	// получения сокета
	//m_sConnectionSocket = socket(m_pAddrInfo->ai_family, m_pAddrInfo->ai_socktype, m_pAddrInfo->ai_protocol);
	m_sConnectionSocket = socket(AF_INET, SOCK_STREAM, NULL);
	if (m_sConnectionSocket == INVALID_SOCKET)
		return ShutdownProcess();

	int result = connect(m_sConnectionSocket, (SOCKADDR*)&addr, sizeof(addr));
	//int result = connect(m_sConnectionSocket, m_pAddrInfo->ai_addr, m_pAddrInfo->ai_addrlen);
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
	} while (amountBytes > 0);

	return result;
}

bool TCPClient::Send(std::string msg)
{
	if (send(m_sConnectionSocket, msg.c_str(), msg.size(), NULL) == SOCKET_ERROR)
		return ShutdownProcess();

	std::cout << "sended\n";

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
	addr.sin_port = htons(1111);
	addr.sin_family = AF_INET;

	//int result = bind(m_sConnectionSocket, m_pAddrInfo->ai_addr, m_pAddrInfo->ai_addrlen);
	int result = bind(m_sConnectionSocket, (SOCKADDR*)&addr, sizeof(addr));
	if (result == SOCKET_ERROR)
		return ShutdownProcess();

	if (listen(m_sConnectionSocket, SOMAXCONN) == SOCKET_ERROR)
		return ShutdownProcess();

	if (isClose)
		return true;

	SOCKET ClientSocket = accept(m_sConnectionSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET)
		return ShutdownProcess();

	m_vSockets.push_back(ClientSocket);
	std::cout << "client connected\n";


	return false;
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

bool TCPServer::StartEcho()
{
	 //std::thread th([this]()
		//{
			char buffer[512];

			if (isClose)
				return true;

	//		while (true)
			{
				for (auto ClientSocket : m_vSockets)
				{
					ZeroMemory(buffer, sizeof(buffer));

					if (recv(ClientSocket, buffer, sizeof(buffer), 0) > 0)
					{
						std::cout << buffer << "\n";
						if (send(ClientSocket, buffer, (int)strlen(buffer), 0) == SOCKET_ERROR)
							return ShutdownProcess();
					}
				}
			}
	//	});

	//th.detach();

	return false;

}

bool TCPServer::StartListening()
{

	//std::thread th([this]()
	//	{
	//		while (true)
			{
			}
	//	});

	//th.detach();

	return false;
}

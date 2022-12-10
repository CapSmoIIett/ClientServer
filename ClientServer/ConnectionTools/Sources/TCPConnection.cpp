#include "../Headers/TCPConnection.h"

#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>
#include <chrono>

#define MEOF "EOF"
#define MSG_SIZE KB * 16


TCPClient::TCPClient() :
#if defined(OS_WINDOWS)
	m_pWsaData(),
#endif
	m_sConnectionSocket(INVALID_SOCKET)
{

}

bool TCPClient::Start()
{
	WIN(m_ConnectionAddress.sin_addr.S_un.S_addr = inet_addr("127.0.0.1"));
	NIX(m_ConnectionAddress.sin_addr.s_addr = INADDR_ANY);

	m_ConnectionAddress.sin_family = AF_INET;
	m_ConnectionAddress.sin_port = htons(2000);

	WIN
	(
		if (WSAStartup(MAKEWORD(2, 2), &m_pWsaData) != 0)
			return false;
	);

	return true;
}

bool TCPClient::Connect(const char* ip)
{
	int result = 0;

	m_sConnectionSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sConnectionSocket == INVALID_SOCKET)
	{
		WIN(printf("Error: Invalid Socket (%d)\n", WSAGetLastError()));
		return false;
	}

	if (ip != nullptr)
	{
		WIN(m_ConnectionAddress.sin_addr.S_un.S_addr = inet_addr(ip));
		NIX(inet_aton(ip, &m_ConnectionAddress.sin_addr));
	}

	result = connect(m_sConnectionSocket, 
		(struct sockaddr*)&m_ConnectionAddress, 
		sizeof(m_ConnectionAddress));
	if (result != 0)
	{
		WIN(printf("Error: Invalid Connection (%d)\n", WSAGetLastError()));
		return false;
	}	

	int flag = 1;
#ifdef OS_WINDOWS
	tcp_keepalive ka { 1, 10 * 1000, 3 * 1000 };

	setsockopt(m_sConnectionSocket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&flag, sizeof(flag));
	//if (setsockopt(ClientSocket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&flag, sizeof(flag)) != 0) 
	//	return false;

	unsigned long numBytesReturned = 0;

	WSAIoctl(m_sConnectionSocket, SIO_KEEPALIVE_VALS, &ka, sizeof(ka), nullptr, 0, &numBytesReturned, 0, nullptr);
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

	setsockopt(m_sConnectionSocket, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag));
	setsockopt(m_sConnectionSocket, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(ka_conf.ka_idle));
	setsockopt(m_sConnectionSocket, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(ka_conf.ka_intvl));
	setsockopt(m_sConnectionSocket, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(ka_conf.ka_cnt));
#endif


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

	WIN(WSACleanup());

	if (m_sConnectionSocket != INVALID_SOCKET)
	{
		shutdown(m_sConnectionSocket, SD_BOTH);

		WIN(closesocket(m_sConnectionSocket));
		NIX(close(m_sConnectionSocket));

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
		WIN(ZeroMemory(recvBuffer, sizeof(recvBuffer)));
		NIX(memset(recvBuffer, 0, sizeof(recvBuffer)));

		WIN
		(
			u_long t = true;
			//ioctlsocket(m_sConnectionSocket, FIONBIO, &t);

			amountBytes = recv(m_sConnectionSocket, recvBuffer, sizeof(recvBuffer), 0);

			t = false;
			//ioctlsocket(m_sConnectionSocket, FIONBIO, &t);
		);
		NIX(amountBytes = recv(m_sConnectionSocket, recvBuffer, sizeof(recvBuffer), MSG_DONTWAIT));

		result += std::string(recvBuffer);
	} while (amountBytes > sizeof(recvBuffer));

	return result;
}

bool TCPClient::Send(std::string msg)
{

	WIN
	(
		if (send(m_sConnectionSocket, msg.c_str(), msg.size(), 0) == SOCKET_ERROR)
			return ShutdownProcess();
	);
	NIX
	(
		if (send(m_sConnectionSocket, msg.c_str(), msg.size(), 0) < 0)
			return ShutdownProcess();
	);

	return true;
}

bool TCPClient::Disconnect()
{
	auto iResult = shutdown(m_sConnectionSocket, SD_SEND);
	if (WIN(iResult == SOCKET_ERROR)NIX(iResult < 0))
	{
		WIN
		(
			printf("shutdown failed: %d\n", WSAGetLastError());
			WSACleanup();
			closesocket(m_sConnectionSocket);
		);

		NIX(close(m_sConnectionSocket));
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
	
		if (sended < 0)
		{
			int err = 0;
			WIN(int)NIX(SockLen_t) len = sizeof(err);
			WIN(err = WSAGetLastError());
			getsockopt(m_sConnectionSocket, SOL_SOCKET, SO_ERROR, (char*) &err, &len);

			WIN(printf("Error: Invalid Listen (%d)\n", WSAGetLastError()));
			return false;
		}

		file.read(buffer, sizeof(buffer));
	}

	WIN(Sleep(1000));
	NIX(sleep(1000));

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
		WIN
		(
			u_long t = true;
			//ioctlsocket(m_sConnectionSocket, FIONBIO, &t);

			len = recv(m_sConnectionSocket, (char*)buffer, sizeof(buffer), 0);

			t = false;
			//ioctlsocket(m_sConnectionSocket, FIONBIO, &t);
		);
		NIX
		(
			len = recv(m_sConnectionSocket, (char*)buffer, sizeof(buffer), MSG_DONTWAIT)
		);

		if (len < 0)
		{
			int err = 0;
			WIN(int)NIX(SockLen_t) len = sizeof(err);
			WIN(err = WSAGetLastError());
			getsockopt(m_sConnectionSocket, SOL_SOCKET, SO_ERROR, (char*) &err, &len);

			WIN(printf("Error: Invalid Listen (%d)\n", WSAGetLastError()));
			return false;
		}

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
	m_sConnectionSocket(INVALID_SOCKET)
{
	isClose = false;
}

bool TCPServer::Start()
{
	WIN(m_ServerAddress.sin_addr.S_un.S_addr = INADDR_ANY);
	NIX(m_ServerAddress.sin_addr.s_addr = INADDR_ANY);

	m_ServerAddress.sin_family = AF_INET;
	m_ServerAddress.sin_port = htons(2000);

	WIN
	(
		if (WSAStartup(MAKEWORD(2, 2), &m_pWsaData) != 0)
			return false
	);

	return true;
}

bool TCPServer::Connect()
{
	int result = 0;

	m_sConnectionSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_sConnectionSocket == INVALID_SOCKET)
	{
		WIN(printf("Error: Invalid Socket (%d)\n", WSAGetLastError()));
		return false;
	}

	result = bind(m_sConnectionSocket, (struct sockaddr*)&m_ServerAddress, sizeof(m_ServerAddress));
	if (WIN(result == SOCKET_ERROR)NIX(result < 0))
	{
		WIN(printf("Error: Invalid Bind (%d)\n", WSAGetLastError()));
		return false;
	}

	result = listen(m_sConnectionSocket, 5);
	if (WIN(result == SOCKET_ERROR)NIX(result < 0))
	{
		WIN(printf("Error: Invalid Listen (%d)\n", WSAGetLastError()));
		return false;
	}

	return true;
}

ConnectedDevice& TCPServer::Access()
{
	WIN(SOCKADDR_IN cs_addr);
	NIX(sockaddr_in cs_addr);

	socklen_t cs_addrsize = sizeof(cs_addr);

	WIN(SOCKET ClientSocket = accept(m_sConnectionSocket, (SOCKADDR*) &cs_addr, &cs_addrsize));
	NIX(int ClientSocket = accept(m_sConnectionSocket, (sockaddr*) &cs_addr, &cs_addrsize));

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

	setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof(flag));
	setsockopt(socket, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(ka_conf.ka_idle));
	setsockopt(socket, IPPROTO_TCP, TCP_KEEPINTVL, &intvl, sizeof(ka_conf.ka_intvl));
	setsockopt(socket, IPPROTO_TCP, TCP_KEEPCNT, &cnt, sizeof(ka_conf.ka_cnt));
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

	WIN(WSACleanup());

	if (m_sConnectionSocket != INVALID_SOCKET)
	{
		shutdown(m_sConnectionSocket, SD_BOTH);

		WIN(closesocket(m_sConnectionSocket));
		NIX(close(m_sConnectionSocket));

		m_sConnectionSocket = INVALID_SOCKET;
	}

	isClose = true;

	for (auto socket : m_vSockets)
	{
		shutdown(socket, SD_BOTH);

		WIN(closesocket(socket));
		NIX(close(socket));

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
		WIN(ZeroMemory(recvBuffer, sizeof(recvBuffer)));
		NIX(memset(recvBuffer, 0, sizeof(recvBuffer)));

#if defined(OS_WINDOWS)
		u_long t = true; 
		//ioctlsocket(m_sConnectionSocket, FIONBIO, &t);

		amountBytes = recv(device.m_Socket, recvBuffer, sizeof(recvBuffer), 0);

		t = false;
		//ioctlsocket(m_sConnectionSocket, FIONBIO, &t);
#else
		amountBytes = recv(device.m_Socket, recvBuffer, sizeof(recvBuffer), MSG_DONTWAIT);
#endif
		
		WIN(if (WSAGetLastError() == WSAECONNRESET || WSAGetLastError() == WSAETIMEDOUT))
		NIX(if (amountBytes < 0))
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
	WIN
	(
		if (send(device.m_Socket, msg.c_str(), msg.size(), 0) == SOCKET_ERROR)
			return ShutdownProcess()
	);
	NIX
	(
		if (send(device.m_Socket, msg.c_str(), msg.size(), 0) < 0)
			return ShutdownProcess()
	);

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

	auto start = std::chrono::system_clock::now();
	//auto clock = std::chrono::high_resolution_clock::now();

	file.read(buffer, sizeof(buffer));
	while ((readed = file.gcount()) != 0)
	{

		sended = send(device.m_Socket, (char*)buffer, readed, 0);
		file.read(buffer, sizeof(buffer));

		WIN(if (WSAGetLastError() == WSAECONNRESET || WSAGetLastError() == WSAETIMEDOUT))
			NIX(if (sended < 0))
		{
			device.m_Status = ConnectedDevice::Status::Disabled;
			counter-= 100;
			device.m_CLInfo = ConnectionLostInfo(ConnectionLostInfo::Status::download, counter * MSG_SIZE, "");
			return false;
		}


		counter++;
	}

	auto end = std::chrono::system_clock::now();
	std::cout << (static_cast<double>(MSG_SIZE * counter) / MB) / 
		std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << "\n";

	WIN(Sleep(1000));
	NIX(sleep(1000));

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

	std::cout << "\n\n";

	auto start = std::chrono::system_clock::now();
	//auto clock = std::chrono::high_resolution_clock::now();

	do
	{
#if defined(OS_WINDOWS)
		u_long t = true; 
		//ioctlsocket(m_sConnectionSocket, FIONBIO, &t);

		len = recv(device.m_Socket, (char*)buffer, sizeof(buffer), 0);

		t = false;
		//ioctlsocket(m_sConnectionSocket, FIONBIO, &t);
#else
		len = recv(device.m_Socket, (char*)buffer, sizeof(buffer), MSG_DONTWAIT);
#endif

		WIN(if (WSAGetLastError() == WSAECONNRESET || WSAGetLastError() == WSAETIMEDOUT))
		NIX(if (len < 0))
		{
			device.m_Status = ConnectedDevice::Status::Disabled;
			counter-= 100;
			device.m_CLInfo = ConnectionLostInfo(ConnectionLostInfo::Status::upload, counter * MSG_SIZE, "");
			return false;
		}

		if (strcmp(buffer, MEOF) == 0)
			break;

		file.write(buffer, len);

		counter++;
	} while (len > 0);

	auto end = std::chrono::system_clock::now();
	std::cout << (static_cast<double>(MSG_SIZE * counter) / MB) / 
		std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << "\n";

	return true;
}

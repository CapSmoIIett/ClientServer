#include "../Headers/OneThread.h"

#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>
#include <chrono>

#define MEOF "EOF"
#define MSG_SIZE KB * 16


OTTCPClient::OTTCPClient() :
#if defined(OS_WINDOWS)
	m_pWsaData(),
#endif
	m_sConnectionSocket(INVALID_SOCKET)
{

}

bool OTTCPClient::Start()
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

bool OTTCPClient::Connect(const char* ip)
{
	int result = 0;

	m_sConnectionSocket = socket(AF_INET, SOCK_STREAM, 0);
	//m_sConnectionSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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

	std::cout << "connected";

	return true;
}


bool OTTCPClient::ShutdownProcess()
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


std::string OTTCPClient::Get()
{
	int amountBytes = 0;
	char recvBuffer[512];
	std::string result;
	WSABUF DataBuf;
	DWORD RecvBytes = 0;
	DWORD flags = 0;

	do
	{
		WIN(ZeroMemory(recvBuffer, sizeof(recvBuffer)));
		NIX(memset(recvBuffer, 0, sizeof(recvBuffer)));

		amountBytes = recv(m_sConnectionSocket, (char*)recvBuffer, sizeof(recvBuffer), 0);
		//WSARecv(m_sConnectionSocket, &DataBuf, 1, &RecvBytes, &flags, NULL, NULL);
		//WSARecv(SI->Socket, &(SI->DataBuf), 1, &RecvBytes, &Flags, &(SI->Overlapped), NULL);

		result += std::string(recvBuffer);
	} while (amountBytes > sizeof(recvBuffer));

	return result;
}

bool OTTCPClient::Send(std::string msg)
{

	WSABUF DataBuf;
	DWORD SendBytes = 0;

	DataBuf.buf = (char*)msg.c_str();
	DataBuf.len = msg.size();

	msg += "\0";

	if (send(m_sConnectionSocket, msg.c_str(), msg.size() + 1, 0) == SOCKET_ERROR)
	//if (WSASend(m_sConnectionSocket, &DataBuf, 1, &SendBytes, 0, &AcceptOverlapped, NULL) == SOCKET_ERROR)
		return false;

	return true;
}

bool OTTCPClient::Disconnect()
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

bool OTTCPClient::SendFile(std::fstream& file)
{
	char buffer[MSG_SIZE]; //выделяем блок 1 Кб
	int sended = 0;
	int readed = 0;

	if (!file.is_open())
		return false;

	Sleep(10);

	file.read(buffer, sizeof(buffer));
	while ((readed = file.gcount()) != 0)
	{
		Send("uploading");

		auto str = Get();
		if (str != "OK")
			continue;

		sended = send(m_sConnectionSocket, (char*)buffer, readed, 0);

		auto str2 = Get();
		if (str != "OK")
			continue;
	
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

	Send("EOF");
	//send(m_sConnectionSocket, (char*)"", sizeof(MEOF), 0);

	return true;
}

bool OTTCPClient::GetFile(std::fstream& file)
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


OTTCPServer::OTTCPServer() :
#if defined(OS_WINDOWS)
	m_pWsaData(),
#endif
	m_sConnectionSocket(INVALID_SOCKET)
{
	isClose = false;
}

bool OTTCPServer::Start()
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

bool OTTCPServer::Connect()
{
	int result = 0;

	m_sConnectionSocket = socket(AF_INET, SOCK_STREAM, 0);
	//m_sConnectionSocket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
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

	ULONG NonBlock = 1;
	ioctlsocket(m_sConnectionSocket, FIONBIO, &NonBlock);
	if (m_sConnectionSocket == INVALID_SOCKET)
	{
		WIN(printf("Error: Invalid Socket (%d)\n", WSAGetLastError()));
		return false;
	}



	return true;
}

bool OTTCPServer::Access()
{
	WIN(SOCKADDR_IN cs_addr);
	NIX(sockaddr_in cs_addr);

	SOCKET ClientSocket;

	FD_ZERO(&ReadSet);
	FD_ZERO(&WriteSet);

	TIMEVAL time;
	time.tv_sec = 0;
	time.tv_usec = 10;

	FD_SET(m_sConnectionSocket, &ReadSet);

	if (select(0, &ReadSet, &WriteSet, NULL, &time) == SOCKET_ERROR)
	{
		WIN(printf("Error: Invalid Socket (%d)\n", WSAGetLastError()));
		return false;
	}



	if (!FD_ISSET(m_sConnectionSocket, &ReadSet))
		return false;

	socklen_t cs_addrsize = sizeof(cs_addr);

	WIN(ClientSocket = accept(m_sConnectionSocket, (SOCKADDR*) &cs_addr, &cs_addrsize));
	NIX(int ClientSocket = accept(m_sConnectionSocket, (sockaddr*) &cs_addr, &cs_addrsize));

	ULONG NonBlock = 1;
	ioctlsocket(ClientSocket, FIONBIO, &NonBlock);
	//CreateSocketInformation(ClientSocket)

	if (ClientSocket == INVALID_SOCKET)
		return false;

	int flag = 1;

	tcp_keepalive ka { 1, 10 * 1000, 3 * 1000 };

	setsockopt(ClientSocket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&flag, sizeof(flag));
	//if (setsockopt(ClientSocket, SOL_SOCKET, SO_KEEPALIVE, (const char*)&flag, sizeof(flag)) != 0) 
	//	return false;

	unsigned long numBytesReturned = 0;

	//WSAIoctl(ClientSocket, SIO_KEEPALIVE_VALS, &ka, sizeof(ka), nullptr, 0, &numBytesReturned, 0, nullptr);
	//if (WSAIoctl(ClientSocket, SIO_KEEPALIVE_VALS, &ka, sizeof(ka), nullptr, 0, &numBytesReturned, 0, nullptr) != 0) 
	//	return false;


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
		return false;
	}

	m_vSockets.push_back(ClientSocket);
	m_vConnectedDevices.push_back(device);

	std::cout << "client connected\n";

	return true;
}

bool OTTCPServer::ShutdownProcess()
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

std::string OTTCPServer::Get(ConnectedDevice& device)
{
	int amountBytes = 0;
	char recvBuffer[512];
	std::string result;

	WSABUF DataBuf;
	DWORD RecvBytes = 0;
	DWORD flags = 0;
	WSAEVENT EventArray[WSA_MAXIMUM_WAIT_EVENTS];
	WSAOVERLAPPED AcceptOverlapped;

	EventArray[0] = WSACreateEvent();

	ZeroMemory(&AcceptOverlapped, sizeof(WSAOVERLAPPED));
	AcceptOverlapped.hEvent = EventArray[0];

	DataBuf.buf = recvBuffer;
	DataBuf.len = sizeof(recvBuffer);

	amountBytes = recv(device.m_Socket, recvBuffer, sizeof(recvBuffer), 0);
	/*if (WSARecv(device.m_Socket, &DataBuf, 1, &RecvBytes, &flags, NULL, NULL) == SOCKET_ERROR)
	{
		return "ERROR";
	}*/
	//amountBytes = recv(device.m_Socket, recvBuffer, sizeof(recvBuffer), 0);

	if (amountBytes > 0)
		result += std::string(recvBuffer);

	return result;
}

bool OTTCPServer::Send(ConnectedDevice& device, std::string msg)
{
	WSABUF DataBuf;
	DWORD SendBytes = 0;

	DataBuf.buf = (char*)msg.c_str();
	DataBuf.len = msg.size();

	if (send(device.m_Socket, msg.c_str(), msg.size(), 0) == SOCKET_ERROR)
	//if (WSASend(m_sConnectionSocket, &DataBuf, 1, &SendBytes, 0, NULL, NULL) == SOCKET_ERROR)
		return false;

	return true;
}

bool OTTCPServer::SendFile(ConnectedDevice& device, std::fstream& file)
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

bool OTTCPServer::GetFile(ConnectedDevice& device, std::fstream& file)
{
	char buffer[MSG_SIZE]; //выделяем блок 1 Кб
	int len = 0;
	int counter = 0;

	WSABUF DataBuf;
	DWORD RecvBytes = 0;

	if (!file.is_open())
		return false;

	DWORD flags = 0;
	//WSARecv(device.m_Socket, &DataBuf, 1, &RecvBytes, &flags, NULL, NULL);
		//len = recv(device.m_Socket, (char*)buffer, sizeof(buffer), 0);
	len = recv(device.m_Socket, (char*)buffer, sizeof(buffer), 0);

	//file.write(DataBuf.buf, DataBuf.len);
	file.write(buffer, len);


	return true;
}

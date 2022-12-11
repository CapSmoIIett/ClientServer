#include "../Headers/UDPConnection.h"

#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>

#define MEOF "EOF"
#define MSG_SIZE KB * 2


// work messages
#define WM_OK  "11"
#define WM_ERR "00"
#define WM_END "22"

#define WM_SIZE sizeof(WM_OK)


UDPClient::UDPClient() :
#if defined(OS_WINDOWS)
	m_pWsaData(),
#endif
	m_sConnectionSocket(INVALID_SOCKET)
{

}

bool UDPClient::Start()
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

bool UDPClient::Connect(const char* ip)
{
	char buf[MSG_SIZE];
	int result = 0;

	WIN(ZeroMemory(buf, sizeof(buf)));
	NIX(memset(buf, 0, sizeof(buf)));

	m_sConnectionSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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

	sendto(m_sConnectionSocket, WM_OK, WM_SIZE, 0, 
		(struct sockaddr*)&m_ConnectionAddress, sizeof(m_ConnectionAddress));

	std::cout << "connected";

	return true;
}


bool UDPClient::ShutdownProcess()
{
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


std::string UDPClient::Get()
{
	int amountBytes = 0;
	char recvBuffer[512];
	char strBytes[sizeof(int)];
	char workMsg[WM_SIZE];
	std::string result, msg;

	while (true)
	{
		do
		{
			WIN(ZeroMemory(recvBuffer, sizeof(recvBuffer)));
			NIX(memset(recvBuffer, 0, sizeof(recvBuffer)));

			socklen_t cs_addrsize = sizeof(m_ConnectionAddress);
			recvfrom(m_sConnectionSocket, recvBuffer, MSG_SIZE, 0, (SOCKADDR*)&m_ConnectionAddress, &cs_addrsize);

			msg += std::string(recvBuffer);
		} while (amountBytes > sizeof(recvBuffer));

		if (strcmp(msg.c_str(), WM_OK) == 0)
			break;

		result = msg;
		msg.clear();

		std::sprintf(strBytes, "%d", result.size());
		sendto(m_sConnectionSocket, strBytes, sizeof(int), 0, 
			(struct sockaddr*)&m_ConnectionAddress, sizeof(m_ConnectionAddress));
	} 

	return result;
}

bool UDPClient::Send(std::string msg)
{
	int amountBytes = 0;
	char workMsg[32];

	do
	{
		sendto(m_sConnectionSocket, msg.c_str(), msg.size(), 0, 
			(struct sockaddr*)&m_ConnectionAddress, sizeof(m_ConnectionAddress));

		socklen_t cs_addrsize = sizeof(m_ConnectionAddress);
		recvfrom(m_sConnectionSocket, workMsg, sizeof(workMsg), 0, (SOCKADDR*)&m_ConnectionAddress, &cs_addrsize);

	} while (atoi(workMsg) != msg.size());

	sendto(m_sConnectionSocket, WM_OK, WM_SIZE, 0, 
		(struct sockaddr*)&m_ConnectionAddress, sizeof(m_ConnectionAddress));

	return true;
}

bool UDPClient::Disconnect()
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

bool UDPClient::SendFile(std::fstream& file)
{
	char workMsg[32];
	int sended = 0;
	int readed = 0;

	int amount = 1;
	const int maxAmount = 10;
	const int maxSize = MSG_SIZE * maxAmount;
	char buffer[maxSize];

	bool isWasError = 0;
	bool isEnd = 0;

	const int waitMaxCount = 10;
	int i = 0;

	socklen_t cs_addrsize = 0;

	if (!file.is_open())
		return false;

	int sizeOfmsg = MSG_SIZE * amount;
	file.read(buffer, sizeOfmsg);
	do
	{
		while (true)
		{

			readed = file.gcount();
			sended = sendto(m_sConnectionSocket, (char*)buffer, readed, 0,
				(struct sockaddr*)&m_ConnectionAddress, sizeof(m_ConnectionAddress));

			if (sended < 0)
			{
				int err = 0;
				WIN(int)NIX(SockLen_t) len = sizeof(err);
				WIN(err = WSAGetLastError());
				getsockopt(m_sConnectionSocket, SOL_SOCKET, SO_ERROR, (char*)&err, &len);

				WIN(printf("Error: Invalid Listen (%d)\n", WSAGetLastError()));
				return false;
			}

			WIN(ZeroMemory(workMsg, sizeof(workMsg)));
			NIX(memset(workMsg, 0, sizeof(workMsg)));

			int msgReaded = 0;
			for (i = 0; i < waitMaxCount; i++)
			{
				cs_addrsize = sizeof(m_ConnectionAddress);
				msgReaded = recvfrom(m_sConnectionSocket, workMsg, sizeof(workMsg), 0,
					(SOCKADDR*)&m_ConnectionAddress, &cs_addrsize);

				if (msgReaded != 0)
				{
					break;
				}
			} 

			if (i == waitMaxCount)
				continue;

			Sleep(1);


			if (isEnd)
			{
				int msgSended = 0;
				for (i = 0; i < waitMaxCount; i++)
				{
					msgSended = sendto(m_sConnectionSocket, WM_END, WM_SIZE, 0,
						(struct sockaddr*)&m_ConnectionAddress, sizeof(m_ConnectionAddress));

					if (msgSended != 0)
						break;
				} 

				if (i == waitMaxCount)
					continue;

				return true;

			}
			if (atoi(workMsg) == sended)
			{
				int msgSended = 0;
				for (i = 0; i < waitMaxCount; i++)
				{
					msgSended = sendto(m_sConnectionSocket, WM_OK, WM_SIZE, 0,
						(struct sockaddr*)&m_ConnectionAddress, sizeof(m_ConnectionAddress));

					if (msgSended != 0)
						break;
				} 

				if (i == waitMaxCount)
					continue;
				break;
			}
			else
			{
				int msgSended = 0;
				for (i = 0; i < waitMaxCount; i++)
				{
					msgSended = sendto(m_sConnectionSocket, WM_ERR, WM_SIZE, 0,
						(struct sockaddr*)&m_ConnectionAddress, sizeof(m_ConnectionAddress));

					if (msgSended != 0)
						break;
				} 

				if (i == waitMaxCount)
					continue;

				isWasError = 1;
			}
		}

		sizeOfmsg = MSG_SIZE * amount;
		file.read(buffer, sizeOfmsg);

		if (isWasError)
		{
			isWasError = 0;
			amount = 1;
		}


		isEnd = file.eof();


	} while ((readed = file.gcount()) != 0);

	sendto(m_sConnectionSocket, WM_END, WM_SIZE, 0,
		(struct sockaddr*)&m_ConnectionAddress, sizeof(m_ConnectionAddress));

	return true;
}

bool UDPClient::GetFile(std::fstream& file)
{
	char strBytes[32];
	char msg[WM_SIZE];
	int getted = 0;
	int len = 0;

	int amount = 1;
	const int maxAmount = 10;
	const int maxSize = MSG_SIZE * maxAmount;
	char buffer[maxSize];

	bool isWasError = 0;

	if (!file.is_open())
		return false;

	do
	{
		do
		{
			int sizeOfmsg = MSG_SIZE * amount;

			socklen_t cs_addrsize = sizeof(m_ConnectionAddress);
			getted = recvfrom(m_sConnectionSocket, (char*)buffer, sizeOfmsg, 0,
				(SOCKADDR*)&m_ConnectionAddress, &cs_addrsize);

			if (getted < 0)
			{
				int err = 0;
				WIN(int)NIX(SockLen_t) len = sizeof(err);
				WIN(err = WSAGetLastError());
				getsockopt(m_sConnectionSocket, SOL_SOCKET, SO_ERROR, (char*)&err, &len);

				WIN(printf("Error: Invalid Listen (%d)\n", WSAGetLastError()));
				return false;
			}

			std::sprintf(strBytes, "%d", getted);
			sendto(m_sConnectionSocket, strBytes, sizeof(strBytes), 0, 
				(struct sockaddr*)&m_ConnectionAddress, sizeof(m_ConnectionAddress));

			WIN(ZeroMemory(msg, sizeof(msg)));
			NIX(memset(msg, 0, sizeof(msg)));

			do
			{
				cs_addrsize = sizeof(m_ConnectionAddress);
				recvfrom(m_sConnectionSocket, (char*)msg, sizeof(msg), 0,
					(SOCKADDR*)&m_ConnectionAddress, &cs_addrsize);
				if (strcmp(msg, WM_ERR) == 0)
				{
					isWasError = 1;
					continue;
				}
			} while (strcmp(msg, WM_OK) != 0 && strcmp(msg, WM_END) != 0);

			if (strcmp(msg, WM_END) == 0)
				return true;

		} while (strcmp(msg, WM_OK) != 0);

		file.write(buffer, len);

		if (isWasError)
		{
			isWasError = 0;
			amount = 1;
		}
	} while (len != 0);

	return true;
}


//-----------------------------------------------------------------------------------------------


UDPServer::UDPServer() :
#if defined(OS_WINDOWS)
	m_pWsaData(),
#endif
	m_sConnectionSocket(INVALID_SOCKET)
{
	isClose = false;
}

bool UDPServer::Start()
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

bool UDPServer::Connect()
{
	int result = 0;

	m_sConnectionSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
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


	return true;
}

ConnectedDevice& UDPServer::Access()
{
	char buf[MSG_SIZE];
	WIN(SOCKADDR_IN cs_addr);
	NIX(sockaddr_in cs_addr);

	WIN(ZeroMemory(buf, sizeof(buf)));
	NIX(memset(buf, 0, sizeof(buf)));

	std::string message;

	do
	{
		socklen_t cs_addrsize = sizeof(cs_addr);
		recvfrom(m_sConnectionSocket, buf, MSG_SIZE, 0, (SOCKADDR*)&cs_addr, &cs_addrsize);

		message = std::string(buf);
	} while (message != WM_OK);

	SOCKET socket = INVALID_SOCKET;
	ConnectedDevice device(socket, cs_addr, ConnectedDevice::Status::Connected);

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

	m_vConnectedDevices.push_back(device);

	std::cout << "client connected\n";

	return device;
}

bool UDPServer::ShutdownProcess()
{

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

std::string UDPServer::Get(ConnectedDevice& device)
{
	int amountBytes = 0;
	char recvBuffer[512];
	char strBytes[sizeof(int)];
	char workMsg[WM_SIZE];
	std::string result, msg;

	while (true)
	{
		do
		{
			WIN(ZeroMemory(recvBuffer, sizeof(recvBuffer)));
			NIX(memset(recvBuffer, 0, sizeof(recvBuffer)));

			socklen_t cs_addrsize = sizeof(device.m_SockAddr);
			recvfrom(m_sConnectionSocket, recvBuffer, sizeof(recvBuffer), 0,
				(SOCKADDR*)&device.m_SockAddr, &cs_addrsize);

			WIN(if (WSAGetLastError() == WSAECONNRESET || WSAGetLastError() == WSAETIMEDOUT))
				NIX(if (amountBytes < 0))
			{
				device.m_Status = ConnectedDevice::Status::Disabled;
				return "";
			}

			msg += std::string(recvBuffer);
		} while (amountBytes > sizeof(recvBuffer));

		if (strcmp(msg.c_str(), WM_OK) == 0)
			break;

		result = msg;
		msg.clear();

		std::sprintf(strBytes, "%d", result.size());
		sendto(m_sConnectionSocket, strBytes, WM_SIZE, 0,
			(struct sockaddr*)&device.m_SockAddr, sizeof(device.m_SockAddr));
	}

	return result;
}

bool UDPServer::Send(ConnectedDevice& device, std::string msg)
{

	int amountBytes = 0;
	char workMsg[32];

	do
	{
	sendto(m_sConnectionSocket, msg.c_str(), msg.size(), 0, 
		(struct sockaddr*)&device.m_SockAddr, sizeof(device.m_SockAddr));

		socklen_t cs_addrsize = sizeof(device.m_SockAddr);
		recvfrom(m_sConnectionSocket, workMsg, sizeof(workMsg), 0,
			(SOCKADDR*)&device.m_SockAddr, &cs_addrsize);

	} while (atoi(workMsg) != msg.size());

	sendto(m_sConnectionSocket, WM_OK, WM_SIZE, 0, 
		(struct sockaddr*)&device.m_SockAddr, sizeof(device.m_SockAddr));

	return true;


	return false;
}

bool UDPServer::SendFile(ConnectedDevice& device, std::fstream& file)
{
	char buffer[MSG_SIZE]; //выделяем блок 1 Кб
	char workMsg[32];
	int readed = 0;
	int sended = 0;
	int amount = 1;

	long long counter = 0;

	socklen_t cs_addrsize = 0;

	if (!file.is_open())
		return false;

	std::cout << "\n\n";

	auto start = std::chrono::system_clock::now();

	file.read(buffer, sizeof(buffer));
	while ((readed = file.gcount()) != 0)
	{
		while (true)
		{
			cs_addrsize = sizeof(device.m_SockAddr);
			sended = sendto(m_sConnectionSocket, (char*)buffer, readed, 0,
				(struct sockaddr*)&device.m_SockAddr, (int)cs_addrsize);

			if (sended < 0)
			{
				WIN(if (WSAGetLastError() == WSAECONNRESET || WSAGetLastError() == WSAETIMEDOUT))
				{
					device.m_Status = ConnectedDevice::Status::Disabled;
					device.m_CLInfo = ConnectionLostInfo(ConnectionLostInfo::Status::download, counter * MSG_SIZE, "");
					return false;
				}
			}

			cs_addrsize = sizeof(device.m_SockAddr);
			recvfrom(m_sConnectionSocket, workMsg, sizeof(workMsg), 0,
				(struct sockaddr*)&device.m_SockAddr, &cs_addrsize);

			if (atoi(workMsg) == sended)
			{
				sendto(m_sConnectionSocket, WM_OK, WM_SIZE, 0,
					(struct sockaddr*)&device.m_SockAddr, (int)cs_addrsize);
				break;
			}
			else
			{
				sendto(m_sConnectionSocket, WM_ERR, WM_SIZE, 0,
					(struct sockaddr*)&device.m_SockAddr, cs_addrsize);
			}
		}

		//std::cout << "|";
		counter += amount;
		file.read(buffer, sizeof(buffer));

		counter++;
	}

	sendto(m_sConnectionSocket, WM_END, WM_SIZE, 0,
		(struct sockaddr*)&device.m_SockAddr, cs_addrsize);

	auto end = std::chrono::system_clock::now();
	std::cout << (static_cast<double>(MSG_SIZE * counter) / MB) / 
		std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << "\n";

	return true;
}

bool UDPServer::GetFile(ConnectedDevice& device, std::fstream& file)
{
	char strBytes[32];
	char msg[WM_SIZE];
	int getted = 0;
	long long counter = 0;

	int amount = 1;
	const int maxAmount = 10;
	const int maxSize = MSG_SIZE * maxAmount;
	char buffer[maxSize];

	const int waitMaxCount = 10;
	int i = 0;

	bool isWasError = 0;

	if (!file.is_open())
		return false;

	auto start = std::chrono::system_clock::now();

	do
	{
		do
		{
			getted = 0;
			int sizeOfmsg = MSG_SIZE * amount;

			socklen_t cs_addrsize = sizeof(device.m_SockAddr);
			getted = recvfrom(m_sConnectionSocket, (char*)buffer, sizeOfmsg, 0,
				(SOCKADDR*)&device.m_SockAddr, &cs_addrsize);

			if (getted < 0)
			{
				WIN(if (WSAGetLastError() == WSAECONNRESET || WSAGetLastError() == WSAETIMEDOUT))
				{
					device.m_Status = ConnectedDevice::Status::Disabled;
					device.m_CLInfo = ConnectionLostInfo(ConnectionLostInfo::Status::upload, counter * MSG_SIZE, "");
					return false;
				}
			}

			if (strcmp(buffer, WM_END) == 0)
				return true;

			int bytesSended = 0;
			for (i = 0; i < waitMaxCount; i++)
			{

				std::sprintf(strBytes, "%d", getted);
				bytesSended = sendto(m_sConnectionSocket, strBytes, sizeof(strBytes), 0,
					(SOCKADDR*)&device.m_SockAddr, (int)cs_addrsize);

				if (bytesSended != 0)
					break;
			} 

			if (i == waitMaxCount)
				continue;

			WIN(ZeroMemory(msg, sizeof(msg)));
			NIX(memset(msg, 0, sizeof(msg)));

			int msgReaded = 0;
			for (i = 0; i < waitMaxCount; i++)
			{
				socklen_t cs_addrsize = sizeof(device.m_SockAddr);
				msgReaded = recvfrom(m_sConnectionSocket, (char*)msg, sizeof(msg), 0,
					(SOCKADDR*)&device.m_SockAddr, &cs_addrsize);

				if (msgReaded != 0)
					break;
			} 

			if (i == waitMaxCount)
				continue;

			std::cout << msg << "\n";
			if (strcmp(msg, WM_ERR) == 0)
			{
				isWasError = 1;
				break;
			}


			if (strcmp(msg, WM_END) == 0)
			{
				file.write(buffer, getted);
				return true;
			}

		} while (strcmp(msg, WM_OK) != 0);

		file.write(buffer, getted);

		counter += amount;


		if (isWasError)
		{
			isWasError = 0;
			amount = 1;
		}

	} while (getted != 0);

	auto end = std::chrono::system_clock::now();
	std::cout << (static_cast<double>(MSG_SIZE * counter) / MB) / 
		std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << "\n";

	return true;
}

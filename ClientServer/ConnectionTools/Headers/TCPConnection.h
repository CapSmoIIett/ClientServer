#pragma once

#include "ConnectionInterface.h"
#include "../../OSDefines.h"
#include "ConnectedDevice.h"

#pragma once

#include <thread>
#include <vector>
#include <fstream>
#include <chrono>

#if defined(OS_WINDOWS)
	//#include <Windows.h>
	#include <WinSock2.h> 
	#include <WS2tcpip.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <unistd.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <arpa/inet.h>
#endif

#ifdef OS_POSIX
	#define SOCKADDR_IN -1
	#define INVALID_SOCKET -1

	#define SD_SEND    SHUT_WR
	#define SD_RECEIVE SHUT_RD
	#define SD_BOTH    SHUT_RDWR
#endif

class TCPClient : public ConnectionInterface
{
public:
	TCPClient();
	virtual bool Start() override;
	virtual bool Connect(const char* ip = "localhost");
	virtual	bool ShutdownProcess() override;

	virtual std::string Get();// override;
	virtual bool Send(std::string);// override;

	bool SendFile(std::fstream& filestream);
	bool GetFile(std::fstream& filestream);

	bool Disconnect();


	bool isConnected()
	{
#ifdef OS_WINDOWS
		return 0 == WSAGetLastError();
#else
		return true;
#endif
	}

protected:

#ifdef OS_WINDOWS
	WSADATA m_pWsaData;
	SOCKET m_sConnectionSocket;
	SOCKADDR_IN m_ConnectionAddress;
#else
	int m_sConnectionSocket;
	sockaddr_in m_ConnectionAddress;
#endif

};




class TCPServer : public ConnectionInterface 
{
public:
	TCPServer();
	virtual bool Start() override;
	bool Connect();
	ConnectedDevice Access();
	virtual	bool ShutdownProcess() override;

	virtual std::string Get(ConnectedDevice&);// override;
	virtual bool Send(ConnectedDevice&, std::string);// override;

	bool SendFile(ConnectedDevice& device, std::fstream& filestream);
	bool GetFile(ConnectedDevice& device, std::fstream& filestream);

	std::vector<ConnectedDevice>& GetConnectedDevices()
	{
		return m_vConnectedDevices;
	}

protected:

#ifdef OS_WINDOWS
	WSADATA m_pWsaData;
	SOCKET m_sConnectionSocket;
	std::vector<SOCKET> m_vSockets;
	SOCKADDR_IN m_ServerAddress;
#else
	int m_sConnectionSocket;
	std::vector<int> m_vSockets;
	sockaddr_in m_ServerAddress;

#endif

	//ADDRINFO* m_pAddrInfo;

	std::vector<ConnectedDevice> m_vConnectedDevices;

	bool isClose;
};

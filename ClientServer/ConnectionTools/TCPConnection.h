#pragma once

#include "ConnectionInterface.h"
#include "../OSDefines.h"

#include <thread>
#include <vector>

#if defined(OS_WINDOWS)
	//#include <Windows.h>
	#include <WinSock2.h> 
	#include <WS2tcpip.h>
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <unistd.h>
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

protected:
	WSADATA m_pWsaData;
	ADDRINFO* m_pAddrInfo;
	SOCKET m_sConnectionSocket;
};

class TCPServer : public ConnectionInterface 
{
public:
	TCPServer();
	virtual bool Start() override;
	bool Connect();
	virtual	bool ShutdownProcess() override;

	bool StartEcho();
	bool StartListening();

protected:
	WSADATA m_pWsaData;
	ADDRINFO* m_pAddrInfo;
	SOCKET m_sConnectionSocket;

	std::vector<SOCKET> m_vSockets;

	bool isClose;
};

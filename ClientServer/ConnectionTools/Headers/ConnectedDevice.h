#pragma once

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
#endif

#include "ConnectionLostInfo.h"


struct ConnectedDevice
{
	enum class ConnectionStatus
	{
		Disabled = 0,
		Connected
	};

	using Status = ConnectionStatus;

	ConnectedDevice(SOCKET& socket, SOCKADDR_IN sockaddr, ConnectionStatus status = Status::Disabled) :
		m_Socket(socket), m_SockAddr(sockaddr), m_Status(status)
	{ };


	SOCKET m_Socket;
	SOCKADDR_IN m_SockAddr;
	ConnectionStatus m_Status;
	ConnectionLostInfo m_CLInfo;

};

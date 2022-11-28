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

#if defined(OS_WINDOWS)
	ConnectedDevice(SOCKET& socket, SOCKADDR_IN sockaddr, ConnectionStatus status = Status::Disabled) :
		m_Socket(socket), m_SockAddr(sockaddr), m_Status(status)
	{ };
#else
	ConnectedDevice(int& socket, SOCKADDR_IN sockaddr, ConnectionStatus status = Status::Disabled) :
		m_Socket(socket), m_SockAddr(sockaddr), m_Status(status)
	{ };
#endif 

#if defined(OS_WINDOWS)
	SOCKET m_Socket;
#else
	int m_Socket;
#endif

#if defined(OS_WINDOWS)
	SOCKADDR_IN m_SockAddr;
#else
	sockaddr_in m_SockAddr;
#endif

	ConnectionStatus m_Status;
	ConnectionLostInfo m_CLInfo;

};

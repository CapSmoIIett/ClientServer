#pragma once

#include "ConnectionInterface.h"
#include "../OSDefines.h"

#include <thread>
#include <vector>
#include <fstream>
#include <chrono>

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

	bool SendFile(std::fstream& filestream);
	bool GetFile(std::fstream& filestream);

	bool Disconnect();

protected:
	WSADATA m_pWsaData;
	ADDRINFO* m_pAddrInfo;
	SOCKET m_sConnectionSocket;
	SOCKADDR_IN m_ConnectionAddress;
};


struct ConnectionLostInfo
{
	enum Status
	{
		none = 0,
		upload,
		download
	};

	ConnectionLostInfo(Status s = none, long b = 0, std::string n = "") :
		m_Status(s), m_iBytesAlredy(b), m_sFileName(n)
	{ };

	std::string Write()
	{
		std::string str;

		str += std::to_string(m_Status) + ",";
		str += std::to_string(m_iBytesAlredy) + ",";
		str += m_sFileName + ",";

		return str;
	}

	void Read(std::string str)
	{
		std::string substr;
		int left = 0;
		int right = 0;

		right = str.find(",");

		m_Status = atoi(str.substr(left, right).c_str());

		left = right;
		right = str.find(",", left + 1);

		m_iBytesAlredy = atoi(str.substr(left + 1, right).c_str());

		left = right;
		right = str.find(",", left + 1);

		if (right - left > 1)
			m_sFileName = str.substr(left + 1, right -left - 1);
		else
			m_sFileName = "";
	}

	int m_Status;
	long m_iBytesAlredy;
	std::string m_sFileName;

};

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

class TCPServer : public ConnectionInterface 
{
public:
	TCPServer();
	virtual bool Start() override;
	bool Connect();
	ConnectedDevice& Access();
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
	WSADATA m_pWsaData;
	ADDRINFO* m_pAddrInfo;
	SOCKET m_sConnectionSocket;
	SOCKADDR_IN m_ServerAddress;

	std::vector<SOCKET> m_vSockets;
	std::vector<ConnectedDevice> m_vConnectedDevices;

	bool isClose;
};

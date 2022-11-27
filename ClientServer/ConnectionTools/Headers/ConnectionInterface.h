#pragma once

#include <string>

class ConnectionInterface
{
public:
	ConnectionInterface();
	virtual bool Start() = 0;
	//bool Connect();
	virtual	bool ShutdownProcess() = 0;

	int&  GetPort();
	std::string GetPortString();

	bool isServer();

private:
	int m_iPort;
	bool m_bIsServer;

};

class ClientInterface 
{
public:
	ConnectionInterface* m_cConnectionTool;


};

class ServerIntergace 
{
public:
	ConnectionInterface* m_cConnectionTool;

	virtual bool StartEcho() = 0;
	virtual bool StartTime() = 0;
};


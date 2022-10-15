#include "ConnectionInterface.h"

ConnectionInterface::ConnectionInterface() :
	m_iPort(0)
{

}

int&  ConnectionInterface::GetPort()
{
	return m_iPort; 
}

std::string ConnectionInterface::GetPortString()
{
	return std::to_string(m_iPort);
}

bool ConnectionInterface::isServer()
{
	return m_bIsServer;
}

// ClientServer.cpp: определяет точку входа для приложения.
//

#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <thread>

#include "../ConnectionTools/TCPConnection.h"

int main()
{
	TCPServer server;
	std::string msg;

	using Commands = std::vector<std::string>;
	std::vector<Commands> commandsPool;

	Commands command;	//
	Commands cmd;		// getted from client

	bool isEcho = true;

	std::cout << "Configure server" << '\n';
	
	// Command 
/*
	do
	{
		command.clear();
		{
			msg.clear();

			std::getline(std::cin, msg);

			std::transform(msg.begin(), msg.end(), msg.begin(),
				[](unsigned char c) { return std::tolower(c); });

			std::istringstream iSStream(msg);

			// нужно добавить функцию проверки команд
			std::string word;
			while (iSStream >> word)
				command.push_back(word);
		}

		if (command.size() <= 0)
			continue;

		if (command[0][0] == 'q')
		{
			return 0;
		}

		if (command[0] == "echo")
		{
			isEcho = true;
		}

	} while (command[0] != "start");
	*/

	server.Start();
	std::cout << "Start server" << '\n';

	server.Connect();

	
	// Command processing
	while (true)
	{
		server.Access();

		server.Send(server.GetConnectedDevices()[0], server.GetConnectedDevices()[0].m_CLInfo.Write());

		if (!server.GetConnectedDevices()[0].m_CLInfo.m_sFileName.empty())
		{
			auto clInfo = server.GetConnectedDevices()[0].m_CLInfo;
			switch (clInfo.m_Status)
			{
			case ConnectionLostInfo::upload:
			{
				std::fstream file;
				file.open(clInfo.m_sFileName, std::fstream::out | std::fstream::app | std::fstream::binary);

				server.GetFile(server.GetConnectedDevices()[0], file);

				if ((server.GetConnectedDevices()[0]).m_Status == ConnectedDevice::Status::Disabled)
				{
					server.GetConnectedDevices()[0].m_CLInfo.m_sFileName = clInfo.m_sFileName;
					server.GetConnectedDevices()[0].m_CLInfo.m_iBytesAlredy += clInfo.m_iBytesAlredy;
					break;
				}

				file.close();
				break;
			}

			case ConnectionLostInfo::download:
			{
				std::fstream file;
				file.open(clInfo.m_sFileName, std::fstream::in | std::fstream::binary);

				file.seekp(clInfo.m_iBytesAlredy, std::ios::beg);

				server.SendFile(server.GetConnectedDevices()[0], file);

				if ((server.GetConnectedDevices()[0]).m_Status == ConnectedDevice::Status::Disabled)
				{
					server.GetConnectedDevices()[0].m_CLInfo.m_sFileName = clInfo.m_sFileName;
					server.GetConnectedDevices()[0].m_CLInfo.m_iBytesAlredy += clInfo.m_iBytesAlredy;
					break;
				}

				file.close();

				break;
			}
			}

		}

		do
		{
			cmd.clear();
			{
				std::string word;
				std::string msgLow;

				if (server.GetConnectedDevices().size() <= 0)
					continue;

				msg = server.Get(server.GetConnectedDevices()[0]);

				if ((server.GetConnectedDevices()[0]).m_Status == ConnectedDevice::Status::Disabled)
					break;

				msgLow = msg;
				std::transform(msgLow.begin(), msgLow.end(), msgLow.begin(),
					[](unsigned char c) { return std::tolower(c); });

				std::istringstream iSStream(msgLow);

				while (iSStream >> word)
					cmd.push_back(word);
			}

			if (cmd[0] == "upload")
			{
				std::fstream file;
				file.open(cmd[1].c_str(), std::fstream::out | std::fstream::trunc | std::fstream::binary);

				server.GetFile(server.GetConnectedDevices()[0], file);

				if ((server.GetConnectedDevices()[0]).m_Status == ConnectedDevice::Status::Disabled)
				{
					server.GetConnectedDevices()[0].m_CLInfo.m_sFileName = cmd[1];
					break;
				}

				file.close();
			}
			else if (cmd[0] == "download")
			{
				std::fstream file;
				file.open(cmd[1].c_str(), std::fstream::in | std::fstream::binary);

				server.SendFile(server.GetConnectedDevices()[0], file);

				if ((server.GetConnectedDevices()[0]).m_Status == ConnectedDevice::Status::Disabled)
				{
					server.GetConnectedDevices()[0].m_CLInfo.m_sFileName = cmd[1];
					break;
				}

				file.close();
			}
			else if (isEcho)
			{
				server.Send(server.GetConnectedDevices()[0], msg);
			}

		} while (cmd[0] != "q");
	}


	server.ShutdownProcess();

	return 0;
}

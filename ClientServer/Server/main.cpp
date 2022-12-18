// ClientServer.cpp: определяет точку входа для приложения.
//

#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <thread>

//#include "../ConnectionTools/Headers/TCPConnection.h"
#include "../ConnectionTools/Headers/OneThread.h"
#include "../ConnectionTools/Headers/UDPConnection.h"

int main()
{
	OTTCPServer server;
	//UDPServer server;
	std::string msg;
	std::fstream file;

	using Commands = std::vector<std::string>;
	std::vector<Commands> commandsPool;

	Commands command;	//
	Commands cmd;		// getted from client

	bool isEcho = true;

	server.Start();
	std::cout << "Start server" << '\n';

	server.Connect();

	// Command processing
	while (true)
	{
		server.Access();
		for (int i = 0; i < server.GetConnectedDevices().size(); i++)
		{
			try
			{

				do
				{
					cmd.clear();

					{
						std::string word;
						std::string msgLow;

						if (server.GetConnectedDevices().size() <= 0)
							continue;

						msg = server.Get(server.GetConnectedDevices()[i]);

						if (msg.empty())
							break;

						if ((server.GetConnectedDevices()[i]).m_Status == ConnectedDevice::Status::Disabled)
							break;

						msgLow = msg;
						std::transform(msgLow.begin(), msgLow.end(), msgLow.begin(),
							[](unsigned char c) { return std::tolower(c); });

						std::istringstream iSStream(msgLow);

						while (iSStream >> word)
							cmd.push_back(word);
					}

					if (cmd.empty())
						continue;

					for (auto str : cmd)
						std::cout << str << " ";
					std::cout << "\n";


					if (cmd[0] == "upload")
					{
						file.open(cmd[1].c_str(), std::fstream::out | std::fstream::trunc | std::fstream::binary);

//						msg = server.Get(server.GetConnectedDevices()[i]);

					}
					else if (cmd[0] == "uploading")
					{
						server.GetFile(server.GetConnectedDevices()[i], file);
						//server.Send(server.GetConnectedDevices()[i], "OK");
					}
					else if (cmd[0] == "download")
					{
						std::fstream file;
						file.open(cmd[1].c_str(), std::fstream::in | std::fstream::binary);

						server.SendFile(server.GetConnectedDevices()[i], file);

						if ((server.GetConnectedDevices()[i]).m_Status == ConnectedDevice::Status::Disabled)
						{
							server.GetConnectedDevices()[i].m_CLInfo.m_sFileName = cmd[1];
							std::cout << "error" << "\n";
							break;
						}

						file.close();
					}
					else if (cmd[0] == "downloading")
					{

					}
					else if (cmd[0] == "EOF")
					{
						file.close();
					}
					else if (isEcho)
					{
						server.Send(server.GetConnectedDevices()[i], msg);
						std::cout << "sended\n";
					}

				} while (!cmd.empty() && cmd[0] != "q");

			}
			catch (...)
			{
				std::cout << "hello" << "\n";
			}
		}
	}


	server.ShutdownProcess();

	return 0;
}

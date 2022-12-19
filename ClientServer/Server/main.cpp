// ClientServer.cpp: определяет точку входа для приложения.
//

#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <thread>

#include "../ConnectionTools/Headers/TCPConnection.h"
#include "../ConnectionTools/Headers/UDPConnection.h"

int main()
{
	//TCPServer server;
	UDPServer server;
	std::string msg;

	using Commands = std::vector<std::string>;
	std::vector<Commands> commandsPool;

	Commands command;	//
	Commands cmd;		// getted from client

	bool isEcho = true;

	server.Start();
	std::cout << "Start server" << '\n';

	server.Connect();
	
	std::vector<STARTUPINFO> SIs;
	std::vector<PROCESS_INFORMATION> PIs;

	// Command processing
	while (true)
	{
		try
		{
			server.Access();

			STARTUPINFO si;
			PROCESS_INFORMATION pi;

			ZeroMemory(&si, sizeof(si));
			ZeroMemory(&pi, sizeof(pi));
			si.cb = sizeof(si);

			if (!CreateProcess(NULL,
				NULL,
				NULL,
				NULL,
				FALSE,
				0,
				NULL,
				NULL,
				&si,
				&pi)
				)
			{
				std::cout << "Create process error.\n";
			}


			SIs.push_back(si);
			PIs.push_back(pi);

			if (!server.GetConnectedDevices()[0].m_CLInfo.m_sFileName.empty())
			{
				CLItoCMD(server.GetConnectedDevices()[0].m_CLInfo, cmd);

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

				if (cmd.empty())
					continue;

				if (cmd[0] == "upload")
				{
					std::fstream file;
					file.open(cmd[1].c_str(), std::fstream::out | std::fstream::trunc | std::fstream::binary);

					server.GetFile(server.GetConnectedDevices()[0], file);

					if ((server.GetConnectedDevices()[0]).m_Status == ConnectedDevice::Status::Disabled)
					{
						server.GetConnectedDevices()[0].m_CLInfo.m_sFileName = cmd[1];
						std::cout << "error" << "\n";
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
						std::cout << "error" << "\n";
						break;
					}

					file.close();
				}
				else if (isEcho)
				{
					server.Send(server.GetConnectedDevices()[0], msg);
					std::cout << "sended\n";
				}

			} while (!cmd.empty() && cmd[0] != "q");

		}
		catch (...)
		{
			std::cout << "hello" << "\n";
		}
	}

	for (pi : PIs)
	{
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}


		server.ShutdownProcess();

	return 0;
}

// ClientServer.cpp: определяет точку входа для приложения.
//

#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <fstream>
#include <thread>


#include "../ConnectionTools/Headers/TCPConnection.h"

int main()
{
	TCPClient client;
	std::string msg;
	std::vector<std::string> commands;

	client.Start();
	std::cout << "client start\n";

	do
	{
		commands.clear();
		{
			msg.clear();

			std::getline(std::cin, msg);
			std::istringstream iSStream(msg);

			std::transform(msg.begin(), msg.end(), msg.begin(),
				[](unsigned char c) { return std::tolower(c); });

			// нужно добавить функцию проверки команд
			std::string word;
			while (iSStream >> word)
				commands.push_back(word);
		}

		if (commands.size() <= 0)
			continue;

		if (commands[0] == "q")
		{
			client.Send("q");
			break;
		}

		if (commands[0] == "connect")
		{
			if (commands.size() < 2)
				commands.push_back("127.0.0.1");

			std::cout << client.Connect(commands[1].c_str()) << "\n";

			auto str = client.Get();

			ConnectionLostInfo clInfo;
			clInfo.Read(str);

			switch (clInfo.m_Status)
			{
			case ConnectionLostInfo::upload:
			{
				std::fstream file;
				file.open(clInfo.m_sFileName, std::fstream::in | std::fstream::binary);

				file.seekp(clInfo.m_iBytesAlredy, std::ios::beg);

				client.SendFile(file);

				file.close();
				break;
			}

			case ConnectionLostInfo::download:
			{
				std::fstream file;
				file.open(clInfo.m_sFileName, std::fstream::out | std::fstream::app | std::fstream::binary);

				client.GetFile(file);

				file.close();
				break;
			}
			}
		}
		else if (commands[0] == "send")
		{
			auto sizeOfCommand = sizeof("send");
			client.Send(msg.substr(sizeOfCommand, msg.size() - sizeOfCommand));
		}
		else if (commands[0] =="download")
		{
			std::fstream file;
			file.open(commands[1].c_str(), std::fstream::out | std::fstream::trunc | std::fstream::binary);

			if (!file.is_open())
				continue;

			client.Send(commands[0] + " " + commands[1]);
			client.GetFile(file);

			file.close();
		}
		else if (commands[0] == "upload")
		{
			std::fstream file;
			file.open(commands[1], std::fstream::in | std::fstream::binary);

			if (!file.is_open())
				continue;

			client.Send(commands[0] + " " + commands[1]);
			client.SendFile(file);

			file.close();
		}
		else if (commands[0] == "get")
		{
			std::cout << client.Get() << "\n";
		}
		else if (commands[0] == "disconnect")
		{
			client.Disconnect();
		}

	} while (commands[0] != "q");

	client.ShutdownProcess();

	return 0;
}

// ClientServer.cpp: определяет точку входа для приложения.
//

#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>


#include "../ConnectionTools/TCPConnection.h"

int main()
{
	bool isEnd = false;
	bool isServer = false;
	std::string msg;

	TCPClient client;

	client.Start();
	std::cout << "client start\n";


	do
	{
		std::vector<std::string> commands;
		msg.clear();

		std::getline(std::cin, msg);
		std::istringstream iSStream(msg);

		std::transform(msg.begin(), msg.end(), msg.begin(),
			[](unsigned char c) { return std::tolower(c); });

		// нужно добавить функцию проверки команд
		std::string word;
		while (iSStream >> word)
			commands.push_back(word);

		if (commands.size() <= 0)
			continue;

		if (commands[0] == "q")
			break;


		if (commands[0] == "connect")
		{
			if (commands.size() < 3)
				continue;

			if (commands[1] != "-ip")
			{
				std::cout << "Error" << "\n";
				continue;
			}

			std::cout << client.Connect(commands[2].c_str()) << "\n";
		}
		else if (commands[0] == "send")
		{
			std::string msg;

			for (int i = 1; i < commands.size(); i++)
				msg += commands[i];

			if (msg.empty())
			{
				std::cout << "Error" << "\n";
				continue;
			}

			client.Send(msg);
		}
		else if (commands[0] == "get")
		{
			std::cout << client.Get() << "\n";
		}

	} while (!isEnd);

	client.ShutdownProcess();

	return 0;
}

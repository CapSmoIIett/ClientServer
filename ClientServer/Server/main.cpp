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

	TCPServer server;

	server.Start();
	server.Connect();
	server.StartListening();

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

		if (commands[0] == "echo")
		{
			server.StartEcho();
		}

	} while (!isEnd);

	server.ShutdownProcess();

	return 0;
}

#include "../Headers/ConnectionLostInfo.h"

void CLItoCMD(const ConnectionLostInfo& cli, std::vector<std::string>& cmd)
{
	switch (cli.m_Status)
	{
	case ConnectionLostInfo::upload:
	{
		cmd.clear();
		cmd.push_back("reupload");
		cmd.push_back(cli.m_sFileName);
		cmd.push_back(std::to_string(cli.m_iBytesAlredy));
		break;
	}
	case ConnectionLostInfo::download:
	{
		cmd.clear();
		cmd.push_back("redownload");
		cmd.push_back(cli.m_sFileName);
		cmd.push_back(std::to_string(cli.m_iBytesAlredy));
		break;
	}
	}

}

#include <string>

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
